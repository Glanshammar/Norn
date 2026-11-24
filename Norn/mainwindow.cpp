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
#include <QFileSystemModel>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QShortcut>
#include <QKeySequence>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QSettings settings(qgetenv("USERNAME"), "Norn");

    // Sidebar button icons
    ui->buttonAI->setIcon(QIcon(":/icons/ai.png"));
    ui->buttonSettings->setIcon(QIcon(":/icons/settings.png"));
    ui->buttonDashboard->setIcon(QIcon(":/icons/dashboard.png"));
    ui->buttonNotes->setIcon(QIcon(":/icons/notes.png"));
    ui->buttonTasks->setIcon(QIcon(":/icons/tasks.png"));

    api = new ApiClient(this);
    ui->TabsWidget->setCurrentWidget(ui->pageDashboard);
    ui->buttonDashboard->setChecked(true);
    ui->notesTabs->setTabsClosable(true);
    if(!settings.contains("firstTime") || settings.value("firstTime").toBool()){
        openFile(":/text/README.md");
        settings.setValue("firstTime", false);
    }
    restoreTabs();

    connect(ui->notesTabs, &QTabWidget::tabCloseRequested, this, [this](int index) {
        QWidget *w = ui->notesTabs->widget(index);
        ui->notesTabs->removeTab(index);
        w->deleteLater();
    });

    auto *closeTabShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this);
    connect(closeTabShortcut, &QShortcut::activated, this, [this]() {
        int index = ui->notesTabs->currentIndex();
        if (index < 0) return;  // no tab open
        QWidget *w = ui->notesTabs->widget(index);
        ui->notesTabs->removeTab(index);
        w->deleteLater();
    });

    auto *saveShortcut = new QShortcut(QKeySequence::Save, ui->notesTabs);
    saveShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(saveShortcut, &QShortcut::activated, this, &MainWindow::onSaveNote);

    // Sidebar button exclusive
    sidebarButtons->setExclusive(true);
    sidebarButtons->addButton(ui->buttonDashboard);
    sidebarButtons->addButton(ui->buttonNotes);
    sidebarButtons->addButton(ui->buttonTasks);
    sidebarButtons->addButton(ui->buttonSettings);

    // Tab page switching keybinds
    ui->buttonDashboard->setShortcut(QKeySequence(Qt::Key_F1));
    ui->buttonNotes->setShortcut(QKeySequence(Qt::Key_F2));
    ui->buttonTasks->setShortcut(QKeySequence(Qt::Key_F3));
    ui->buttonSettings->setShortcut(QKeySequence(Qt::Key_F4));

    // Set notes list to path
    QString documentsFolder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    fileModel = new QFileSystemModel(this);
    fileModel->setRootPath(documentsFolder);
    fileModel->setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    ui->listNotes->setModel(fileModel);
    ui->listNotes->setRootIndex(fileModel->index(documentsFolder));

    for (int i = 1; i < fileModel->columnCount(); ++i) {
        ui->listNotes->hideColumn(i);
    }

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


void MainWindow::on_buttonSettings_clicked()
{
    ui->TabsWidget->setCurrentWidget(ui->pageSettings);
}


void MainWindow::on_listNotes_doubleClicked(const QModelIndex &index)
{
    openFile(fileModel->filePath(index));
}

void MainWindow::openFile(const QString &path)
{
    QFileInfo info(path);
    const QString fileType = info.completeSuffix();

    const QList<QString> allowedFileTypes = {"txt", "md"};
    if (!allowedFileTypes.contains(fileType)) {
        qDebug() << "File is not a text file.";
        return;
    }

    // Switch to already open file
    QString wanted = info.canonicalFilePath();
    if (wanted.isEmpty())
        wanted = info.absoluteFilePath();

    for (int i = 0; i < ui->notesTabs->count(); ++i) {
        QWidget *existingTab = ui->notesTabs->widget(i);
        if (!existingTab) continue;

        const QString existingPath = existingTab->property("filePath").toString();
        if (existingPath.isEmpty()) continue;

        QString existing = QFileInfo(existingPath).canonicalFilePath();
        if (existing.isEmpty())
            existing = QFileInfo(existingPath).absoluteFilePath();

#ifdef Q_OS_WIN
        const bool same = existing.compare(wanted, Qt::CaseInsensitive) == 0;
#else
        const bool same = (existing == wanted);
#endif
        if (same) {
            ui->notesTabs->setCurrentIndex(i);
            if (QTextEdit *te = existingTab->findChild<QTextEdit *>())
                te->setFocus();
            return;
        }
    }
    // ------------------------------------------------

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "File couldn't be read:" << path;
        return;
    }

    QWidget *tabPage = new QWidget;
    QGridLayout *gridLayout = new QGridLayout(tabPage);

    QTextEdit *textEdit = new QTextEdit;
    textEdit->setFontPointSize(12);
    textEdit->setCursor(Qt::IBeamCursor);

    gridLayout->addWidget(textEdit, 0, 0);
    tabPage->setProperty("filePath", info.filePath());
    ui->notesTabs->addTab(tabPage, info.fileName());
    int tabIndex = ui->notesTabs->indexOf(tabPage);
    ui->notesTabs->setCurrentIndex(tabIndex);

    const QString contents = QString::fromUtf8(file.readAll());
    if (fileType == "txt")
        textEdit->setPlainText(contents);
    else
        textEdit->setMarkdown(contents);
}

void MainWindow::restoreTabs()
{
    QSettings settings(qgetenv("USERNAME"), "Norn");
    QStringList openFiles = settings.value("openFiles").toStringList();
    int currentIndex = settings.value("currentTab", 0).toInt();

    qDebug() << "All files: " << openFiles;

    for (const QString &path : openFiles) {
        qDebug() << "Opening file: " << path;
        openFile(path);
    }

    ui->notesTabs->setCurrentIndex(currentIndex);
}

void MainWindow::onSaveNote()
{
    const int idx = ui->notesTabs->currentIndex();
    if (idx < 0) { qDebug() << "No note tab is active."; return; }

    QWidget *tab = ui->notesTabs->widget(idx);
    if (!tab) return;

    QTextEdit *editor = tab->findChild<QTextEdit *>();
    if (!editor) return;

    const QString path = tab->property("filePath").toString();
    if (path.isEmpty()) {
        qDebug() << "Active editor has no filePath (e.g. the default tab).";
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qDebug() << "Cannot write note:" << path << "(" << file.errorString() << ")";
        return;
    }

    if (QFileInfo(path).completeSuffix() == "md")
        file.write(editor->toMarkdown().toUtf8());
    else
        file.write(editor->toPlainText().toUtf8());

    file.close();
    qDebug() << "Saved note:" << path;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings(qgetenv("USERNAME"), "Norn");

    QStringList openFiles;
    for (int i = 0; i < ui->notesTabs->count(); ++i) {
        openFiles << ui->notesTabs->widget(i)->property("filePath").toString();
    }

    settings.setValue("openFiles", openFiles);
    settings.setValue("currentTab", ui->notesTabs->currentIndex());

    qDebug() << "Settings saved: " << settings.value("openFiles") << " " << settings.value("currentTab");

    QMainWindow::closeEvent(event);
}