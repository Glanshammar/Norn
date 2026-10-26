#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QVariant>
#include <QPair>
#include "apiclient.h"
#include "databasemanager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStatusReceived(const QPair<QString, QString> &pair);
    void onStatusError(const QString &error);

    void onPostResponse(const QByteArray &response);
    void onPostError(const QString &error);
    void ApiTest();
    void DatabaseTest();

    void on_actionExit_triggered();
    void on_buttonDashboard_clicked();
    void on_buttonNotes_clicked();
    void on_buttonTasks_clicked();
    void on_buttonAffiliates_clicked();

private:
    Ui::MainWindow *ui;
    ApiClient *api;
    void updateTokenDisplay();
    DatabaseManager& db = DatabaseManager::instance();

signals:
    void apiResponseReceived(const QJsonDocument& response);
    void apiErrorReceived(const QString& error);
};
#endif // MAINWINDOW_H
