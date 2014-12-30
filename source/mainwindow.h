#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>
#include <QtGui/QProgressDialog>

#include <QtCore/QScopedPointer>
#include <QtCore/QStringList>
#include <QtCore/QVector>
#include <QtCore/QPair>
#include <QtCore/QMap>

#include "downloadmanager.h"

namespace Ui {
class MainWindow;
}

class QStringListModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void changeIndicators(int index);
    void updateConfiguration();
    void updateFinished();
    void selectFolder();
    void downloadXML();
    void downloadCSV();
    void displayErrorToUser(QString);
    void makeCSV(QString);

private:
    bool loadConfiguration();
    bool loadTopics();
    bool loadIndicators();
    bool loadCountries();
    bool validateInput();
    QUrl prepareUrlAndFilename(QString& filename) const;

private:
    QScopedPointer<Ui::MainWindow>  ui;
    QScopedPointer<QProgressDialog> m_progress_dlg;

    QScopedPointer<QStringListModel> m_topics_model;
    QScopedPointer<QStringListModel> m_indicators_model;
    QScopedPointer<QStringListModel> m_countries_model;

    QVector<QStringList>   m_topics_indicators;
    QMap<QString, QString> m_indicator_id;
    QVector<QString>       m_country_code;

    DownloadManager manager;
};

#endif // MAINWINDOW_H
