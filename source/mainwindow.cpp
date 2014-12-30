#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtGui/QStringListModel>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtCore/QThread>
#include <QtCore/QDebug>
#include <QtCore/QDate>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QMap>
#include <QtXml/QDomDocument>
#include <QtXml/QDomNodeList>
#include <QtXml/QDomElement>

#define INDICATORS_URL       "http://api.worldbank.org/indicator?featured=1&per_page=32767"
#define TOPICS_URL           "http://api.worldbank.org/topics?per_page=32767"
#define COUNTRIES_URL        "http://api.worldbank.org/countries?per_page=32767"

#define TOPICS_XML_FILE      "/config/topics.xml"
#define INDICATORS_XML_FILE  "/config/indicator.xml"
#define COUNTRIES_XML_FILE   "/config/countries.xml"

#define WORLD_BANK_NAMESPACE "wb"
#define VALUE                "value"
#define NAME                 "name"
#define ID                   "id"
#define TOPICS               "topics"
#define INDICATOR            "indicator"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow())
    , m_progress_dlg(new QProgressDialog())
    , m_topics_model(new QStringListModel())
    , m_indicators_model(new QStringListModel())
    , m_countries_model(new QStringListModel())
{
    ui->setupUi(this);

    this->setFixedSize(this->size());

    if (loadConfiguration())
    {
        ui->TopicCmbBox->setModel(m_topics_model.data());
        ui->IndicatorCmbBox->setModel(m_indicators_model.data());
        ui->CountiesLstView->setModel(m_countries_model.data());
    }
    else
    {
        ui->GetCSVBtn->setEnabled(false);
        ui->GetXMLBtn->setEnabled(false);

        QMessageBox::warning(this, "Start up error", "Configuration files are corrupted or do not exist. Please press OK and update the program.");
    }

    ui->FromDateEdit->setMaximumDate(QDate::currentDate());
    ui->TillDateEdit->setMaximumDate(QDate::currentDate());

    QObject::connect(&manager, SIGNAL(progress(int)), m_progress_dlg.data(), SLOT(setValue(int)));
    QObject::connect(m_progress_dlg.data(), SIGNAL(canceled()), &manager, SLOT(cancelAllDownloads()));
    QObject::connect(&manager, SIGNAL(error(QString)), this, SLOT(displayErrorToUser(QString)));
    QObject::connect(&manager, SIGNAL(downloadFinished(QString)), this, SLOT(makeCSV(QString)));
}

MainWindow::~MainWindow()
{
}

bool MainWindow::loadConfiguration()
{
    return (loadTopics() && loadIndicators() && loadCountries());
}

bool MainWindow::loadTopics()
{
    QFile file(QDir::currentPath().append(TOPICS_XML_FILE));

    if (! file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "loadTopics() cannot open file";
        return false;
    }

    QDomDocument doc("init");
    {
        QString err_msg;
        int line, col;
        if (!doc.setContent(&file, false, &err_msg, &line, &col))
        {
            qDebug() << "loadTopics() PARSE err: " << err_msg << "\tline: " << line << "\tcol: " << col;
            return false;
        }
    }

    QDomElement root = doc.documentElement();
    QDomNodeList topic_nodes = root.childNodes();
    QStringList topics;

    for (int i = 0, size = topic_nodes.count(); i < size; ++i)
    {
        QDomNodeList elem = topic_nodes.at(i).toElement().elementsByTagName(WORLD_BANK_NAMESPACE":"VALUE);
        topics << elem.at(0).toElement().text();
    }

    m_topics_model->setStringList(topics);
    m_topics_indicators.fill(QStringList(), topics.count());

    return true;
}

bool MainWindow::loadIndicators()
{
    QFile file(QDir::currentPath().append(INDICATORS_XML_FILE));

    if (! file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "loadIndicators() cannot open file";
        return false;
    }

    QDomDocument doc("init");
    {
        QString err_msg;
        int line, col;
        if (!doc.setContent(&file, false, &err_msg, &line, &col))
        {
            qDebug() << "loadIndicators() PARSE err: " << err_msg << "\tline: " << line << "\tcol: " << col;
            return false;
        }
    }

    QDomElement root = doc.documentElement();
    QDomNodeList indicators_node = root.childNodes();

    for (int i = 0, size = indicators_node.count(); i < size; ++i)
    {
        QDomElement elem = indicators_node.at(i).toElement();
        QString indicator_id = elem.attributeNode(ID).value();
        QString indicator_name = elem.elementsByTagName(WORLD_BANK_NAMESPACE":"NAME).at(0).toElement().text();
        m_indicator_id[indicator_name] = indicator_id;

        QDomNodeList topics = elem.elementsByTagName(WORLD_BANK_NAMESPACE":"TOPICS).at(0).childNodes();
        for (int j = 0, size_2 = topics.count(); j < size_2; ++j)
        {
            QDomElement elem_2 = topics.at(j).toElement();
            int topic_id = elem_2.attributeNode(ID).value().toInt();

            if (--topic_id < m_topics_indicators.count())
                m_topics_indicators[topic_id] << indicator_name;
            else
                return false;
        }
    }

    m_indicators_model->setStringList(m_topics_indicators[0]);

    return true;
}

bool MainWindow::loadCountries()
{
    QFile file(QDir::currentPath().append(COUNTRIES_XML_FILE));

    if (! file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "loadCountries() cannot open file";
        return false;
    }

    QDomDocument doc("init");
    {
        QString err_msg;
        int line, col;
        if (!doc.setContent(&file, false, &err_msg, &line, &col))
        {
            qDebug() << "loadCountries() PARSE err: " << err_msg << "\tline: " << line << "\tcol: " << col;
            return false;
        }
    }

    QDomElement root = doc.documentElement();
    QDomNodeList country_nodes = root.childNodes();
    QStringList countries;

    for (int i = 0, size = country_nodes.count(); i < size; ++i)
    {
        QDomElement elem = country_nodes.at(i).toElement();
        m_country_code << elem.elementsByTagName(WORLD_BANK_NAMESPACE":iso2Code").at(0).toElement().text();
        countries << elem.elementsByTagName(WORLD_BANK_NAMESPACE":"NAME).at(0).toElement().text();
    }

    m_countries_model->setStringList(countries);

    return true;
}

bool MainWindow::validateInput()
{
    if (ui->TopicCmbBox->currentIndex() < 0)
    {
        QMessageBox::critical(this, QObject::trUtf8("Input error"), QObject::trUtf8("No one topic is not selected. Please select a topic first."));
        return false;
    }

    if (ui->IndicatorCmbBox->currentIndex() < 0)
    {
        QMessageBox::critical(this, QObject::trUtf8("Input error"), QObject::trUtf8("No one indicator is not selected. Please select an indicator."));
        return false;
    }

    QItemSelectionModel* model = ui->CountiesLstView->selectionModel();
    QModelIndexList lst = model->selectedRows();

    if (lst.empty())
    {
        QMessageBox::critical(this, QObject::trUtf8("Input error"), QObject::trUtf8("No one country is not selected. Please select a country(ies)."));
        return false;
    }

    if (ui->lineEdit->text().isEmpty() || !QDir(ui->lineEdit->text()).exists())
    {
        QMessageBox::critical(this, QObject::trUtf8("Input error"), QObject::trUtf8("Please select existing folder for saving files."));
        return false;
    }

    QDir dir(ui->lineEdit->text());

    if (!dir.exists())
    {
        QMessageBox::critical(this, QObject::trUtf8("Input error"), QObject::trUtf8("Please select existing folder for saving files."));
        return false;
    }

    return true;
}

QUrl MainWindow::prepareUrlAndFilename(QString& filename) const
{
    QItemSelectionModel* model = ui->CountiesLstView->selectionModel();
    QModelIndexList lst = model->selectedRows();

    QString path("countries/");

    for (int i = 0; i < lst.count(); ++i)
    {
        path += QString("%1;").arg(m_country_code[lst.at(i).row()]);
    }

    path.chop(1);

    int topic = ui->TopicCmbBox->currentIndex();
    int indicator = ui->IndicatorCmbBox->currentIndex();

    QString indicator_id = m_indicator_id[m_topics_indicators[topic][indicator]];
    path += "/indicators/";
    path += indicator_id;
    indicator_id.replace('.', '_');
    filename = QString("%1_").arg(indicator_id);

    QDate from = ui->FromDateEdit->date();
    QDate till = ui->TillDateEdit->date();

    if (till < from)
        qSwap(from, till);

    QString date = QString("%1:%2").arg(from.year()).arg(till.year());
    filename += QString("%1_%2_%3.").arg(from.year()).arg(till.year()).arg(qrand());

    QUrl url;
    url.setScheme("http");
    url.setHost("api.worldbank.org");
    url.setPath(path);

    QList<QPair<QString, QString> > list;
    list.push_back(qMakePair(QString("format"), QString("xml")));
    list.push_back(qMakePair(QString("per_page"), QString("32767")));
    list.push_back(qMakePair(QString("date"), date));

    url.setQueryItems(list);

    return url;
}

void MainWindow::changeIndicators(int index)
{
    if (index >= 0 && m_indicators_model != 0)
    {
        m_indicators_model->setStringList(m_topics_indicators[index]);
    }
}

void MainWindow::updateConfiguration()
{
    QObject::connect(&manager, SIGNAL(allDownloadsFinished()), this, SLOT(updateFinished()));
    QObject::disconnect(&manager, SIGNAL(downloadFinished()), this, SLOT(makeCSV(QString)));

    QDir dir(QDir::currentPath());

    if (!dir.cd("config"))
        dir.mkdir("config");

    manager.doDownload(QUrl::fromEncoded(INDICATORS_URL), QDir::currentPath().append(INDICATORS_XML_FILE));
    manager.doDownload(QUrl::fromEncoded(TOPICS_URL)    , QDir::currentPath().append(TOPICS_XML_FILE));
    manager.doDownload(QUrl::fromEncoded(COUNTRIES_URL) , QDir::currentPath().append(COUNTRIES_XML_FILE));
}

void MainWindow::updateFinished()
{
    QMessageBox::information(this, "Updates finished", "Config files succefully updated. Please restart the program.");
    QObject::disconnect(&manager, SIGNAL(allDownloadsFinished()), this, SLOT(updateFinished()));
    QObject::connect(&manager, SIGNAL(downloadFinished()), this, SLOT(makeCSV(QString)));
}

void MainWindow::selectFolder()
{
    ui->lineEdit->setText(QFileDialog::getExistingDirectory(this, "Select folder", QDir::currentPath()));
}

void MainWindow::downloadXML()
{
    if (!validateInput())
        return;

    QString filename;
    QUrl url = prepareUrlAndFilename(filename);
    filename += "xml";
    QDir dir(ui->lineEdit->text());
    manager.doDownload(url, dir.absoluteFilePath(filename));
}

void MainWindow::downloadCSV()
{
    if (!validateInput())
        return;

    QString filename;
    QUrl url = prepareUrlAndFilename(filename);
    filename += 't';
    QDir dir(ui->lineEdit->text());
    manager.doDownload(url, dir.absoluteFilePath(filename));
}

void MainWindow::displayErrorToUser(QString msg)
{
    QMessageBox::critical(this, QObject::trUtf8("Network error"), msg);
}

void MainWindow::makeCSV(QString filename)
{
    if (!filename.endsWith('t'))
        return;

    QFile in(filename);

    filename.chop(1);
    filename += "csv";

    QFile out(filename);

    if (!in.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "makeCSV: cannot open file for input";
        return;
    }
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "makeCSV: cannot open file for output";
        return;
    }

    QDomDocument doc;
    {
        QString msg;
        int line, col;
        if (!doc.setContent(&in, &msg, &line, &col))
        {
            qDebug() << "makeCSV: parse error: " << msg;
            qDebug() << line << '\t' << col;
            return;
        }
    }

    QDomElement root = doc.documentElement();
    QDomNodeList result_nodes = root.childNodes();
    QMap<QString, QMap<uint, QString> > map;

    for (int i = 0, size = result_nodes.count(); i < size; ++i)
    {
        QDomElement elem = result_nodes.at(i).toElement();
        QMap<uint, QString>& m = map[elem.elementsByTagName(WORLD_BANK_NAMESPACE":country").at(0).toElement().text()];
        uint year = elem.elementsByTagName(WORLD_BANK_NAMESPACE":date").at(0).toElement().text().toUInt();
        QString value = elem.elementsByTagName(WORLD_BANK_NAMESPACE":value").at(0).toElement().text();
        m[year] = (value.isEmpty() ? "null" : value.replace('.', ','));
    }

    QTextStream stream(&out);

    {
        QList<uint> keys = map.begin()->keys();

        stream << "Countries;";
        for (int i = 0; i < keys.count(); ++i)
            stream << keys[i] << ';';
        stream << '\n';
    }

    for (QMap<QString, QMap<uint, QString> >::const_iterator i = map.begin(), end = map.end(); i != end; ++i)
    {
        stream << i.key() << ';';
        for (QMap<uint, QString>::const_iterator j = i.value().begin(), end2 = i.value().end(); j != end2; ++j)
        {
            stream << j.value() << ';';
        }
        stream << '\n';
    }

    in.remove();
}
