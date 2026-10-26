#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDebug>
#include <QIcon>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>
#include <QMessageBox>
#include <QSqlRecord>
#include <QSqlQuery>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->TabsWidget->setCurrentWidget(ui->pageDashboard);
    ui->buttonAI->setIcon(QIcon(":/icons/ai.png"));
    ui->buttonSettings->setIcon(QIcon(":/icons/settings.png"));
    ui->buttonDashboard->setIcon(QIcon(":/icons/dashboard.png"));
    ui->buttonNotes->setIcon(QIcon(":/icons/notes.png"));
    ui->buttonTasks->setIcon(QIcon(":/icons/tasks.png"));
    api = new ApiClient(this);

    // Tab page switching keybinds
    ui->buttonDashboard->setShortcut(QKeySequence(Qt::Key_F1));
    ui->buttonNotes->setShortcut(QKeySequence(Qt::Key_F2));
    ui->buttonTasks->setShortcut(QKeySequence(Qt::Key_F3));
    ui->buttonAffiliates->setShortcut(QKeySequence(Qt::Key_F4));

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

    if (!db.open("localhost", 5432, "norn_dev", "postgres", "postgres")) {
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

void MainWindow::tokenDisplay(QLabel& label) {
    QString token = api->bearerToken();
    if (token.isEmpty()) {
        label.setText("No token set");
        label.setStyleSheet("color: red;");
    } else {
        label.setText("Token: " + token);
        label.setStyleSheet("color: green;");
    }
}

void MainWindow::onStatusReceived(const QPair<QString, QString> &pair) {
    QMessageBox::information(this, "Status: " + pair.first, pair.second);
}

void MainWindow::onStatusError(const QString &error) {
    QMessageBox::warning(this, "Error", error);
}

void MainWindow::onPostResponse(const QByteArray &response) {
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        // ui->textEdit->setPlainText("JSON Parse Error: " + parseError.errorString());
        return;
    }

    QString formattedJson = jsonDoc.toJson(QJsonDocument::Indented);
    // ui->textEdit->setPlainText(formattedJson);

    // ui->labelPage1->setText("POST successful!");
    // ui->labelPage1->setStyleSheet("color: green;");
}

void MainWindow::onPostError(const QString &error) {
    // ui->textEdit->setPlainText("POST Error: " + error);
    // ui->labelPage1->setText("POST failed!");
    // ui->labelPage1->setStyleSheet("color: red;");
}

void MainWindow::ApiTest()
{
    api->get(QUrl("http://127.0.0.1:5285/api/status"),
            [this](const QByteArray &data) {
                qDebug() << "Got data:" << data;

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

                qDebug() << "Status:" << status;
                qDebug() << "Message:" << message;
                // ui->labelPage1->setText("Status: " + status + "\n" + "Message: " + message);
            },
            [this](const QString &error) {
                qDebug() << "Error:" << error;
                // ui->labelPage1->setText("Error: " + error);
            });
}


void MainWindow::DatabaseTest()
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

void MainWindow::on_buttonDashboard_clicked()
{
    ui->TabsWidget->setCurrentWidget(ui->pageDashboard);
}


void MainWindow::on_buttonNotes_clicked()
{
    ui->TabsWidget->setCurrentWidget(ui->pageNotes);
}


void MainWindow::on_buttonTasks_clicked()
{
    ui->TabsWidget->setCurrentWidget(ui->pageTasks);
}


void MainWindow::on_buttonAffiliates_clicked()
{
    ui->TabsWidget->setCurrentWidget(ui->pageAffiliates);
}

