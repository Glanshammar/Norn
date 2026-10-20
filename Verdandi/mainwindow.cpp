#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDebug>
#include <QIcon>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>

#include <QSqlRecord>
#include <QSqlQuery>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->TabsWidget->setCurrentIndex(0);
    api = new ApiClient(this);

    qDebug() << "Thread before invokeMethod:" << QThread::currentThread();
    qDebug() << "ApiClient thread:" << this->thread();

    //API Status
    connect(api, &ApiClient::statusReady,
            this, &MainWindow::onStatusReceived);
    connect(api, &ApiClient::statusError,
            this, &MainWindow::onStatusError);

    //API Post
    connect(api, &ApiClient::postResponseReceived,
            this, &MainWindow::onPostResponse);
    connect(api, &ApiClient::postError,
            this, &MainWindow::onPostError);

    updateTokenDisplay();

    if (!db.open("localhost", 5432, "verdandi_dev", "postgres", "postgres")) {
        qDebug() << "Cannot open database:" << db.lastError();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionExit_triggered()
{
    QApplication::quit();
}

void MainWindow::updateTokenDisplay() {
    QString token = api->bearerToken();
    if (token.isEmpty()) {
        ui->labelPage2->setText("No token set");
        ui->labelPage2->setStyleSheet("color: red;");
    } else {
        // Show only first/last few chars for security
        QString displayToken = token.length() > 20
                                   ? token.left(10) + "..." + token.right(10)
                                   : token;
        ui->labelPage2->setText("Token: " + displayToken);
        ui->labelPage2->setStyleSheet("color: green;");
    }
}

void MainWindow::onStatusReceived(const QPair<QString, QString> &pair) {
    ui->labelPage1->setText("Status: " + pair.first + "\n" + "Message: " + pair.second);
}

void MainWindow::onStatusError(const QString &error) {
    ui->labelPage1->setText("Error: " + error);
}

void MainWindow::onPostResponse(const QByteArray &response) {
    // Parse and display POST response
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        ui->textEdit->setPlainText("JSON Parse Error: " + parseError.errorString());
        return;
    }

    // Pretty print JSON
    QString formattedJson = jsonDoc.toJson(QJsonDocument::Indented);
    ui->textEdit->setPlainText(formattedJson);

    // Also update a label
    ui->labelPage1->setText("POST successful!");
    ui->labelPage1->setStyleSheet("color: green;");
}

void MainWindow::onPostError(const QString &error) {
    ui->textEdit->setPlainText("POST Error: " + error);
    ui->labelPage1->setText("POST failed!");
    ui->labelPage1->setStyleSheet("color: red;");
}

void MainWindow::on_buttonApiTest_clicked()
{
    api->get(QUrl("http://127.0.0.1:5285/api/status"),
            [this](const QByteArray &data) {  // Capture 'this' to access member variables
                qDebug() << "Got data:" << data;

                // Parse JSON
                QJsonParseError parseError;
                QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);

                if (parseError.error != QJsonParseError::NoError) {
                    qDebug() << "JSON Parse Error:" << parseError.errorString();
                    return;
                }

                if (!jsonDoc.isObject()) {
                    qDebug() << "JSON is not an object";
                    return;
                }

                QJsonObject jsonObj = jsonDoc.object();
                QString status = jsonObj["status"].toString();
                QString message = jsonObj["message"].toString();

                ui->labelPage1->setText("Status: " + status + "\n" + "Message: " + message);
            },
            [this](const QString &error) {  // Capture 'this' for error handling
                qDebug() << "Error:" << error;
                ui->labelPage1->setText("Error: " + error);
            });
}

void MainWindow::on_buttonApiTest2_clicked()
{
    api->getStatus();
}

void MainWindow::on_buttonApiPost_clicked()
{

}


void MainWindow::on_buttonPage2_triggered(QAction *arg1)
{
    ui->TabsWidget->setCurrentIndex(1);
}


void MainWindow::on_buttonDashboard_triggered(QAction *arg1)
{
    ui->TabsWidget->setCurrentIndex(0);
}


void MainWindow::on_buttonDashboard_clicked()
{
    ui->TabsWidget->setCurrentIndex(0);
}


void MainWindow::on_buttonPage2_clicked()
{
    ui->TabsWidget->setCurrentIndex(1);
}


void MainWindow::on_buttonDatabase_clicked()
{
    if (!db.createTableBuilder("departments")
             .id()
             .text("name", /*notNull=*/true)
             .create()) {
        qDebug() << "Failed to create departments:" << db.lastError();
    } else {
        qDebug() << "Created 'departments' table in database.";
    }

    if (!db.createTableBuilder("employees")
             .id()
             .text("name", true)
             .integer("department_id")
             .numeric("salary")
             .timestamp("hired_at")
             .foreignKey("department_id", "departments", "id")
             .create()) {
        qDebug() << "Failed to create employees:" << db.lastError();
    } else {
        qDebug() << "Created 'employees' table in database.";
    }
}