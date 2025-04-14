/**
 * @file dbwindow.cpp
 * @brief Implementation of the database browser window.
 */

#include "dbwindow.h"
#include "ui_dbwindow.h"
#include "db.h"
#include "mainwindow.h"

/**
 * @brief Constructs the database browser window.
 * @details Initializes UI elements including:
 * - City selection area
 * - Scrollable file list
 * - Sets window properties
 */
dbWindow::dbWindow(MainWindow *mainWindow, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::dbWindow)
    , m_mainWindow(mainWindow)
{
    ui->setupUi(this);

    setWindowTitle("Saved Database Records");
    resize(500, 400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *label = new QLabel("Select city:");
    mainLayout->addWidget(label);

    QWidget *cityContainer = new QWidget();
    cityLayout = new QVBoxLayout(cityContainer);
    mainLayout->addWidget(cityContainer);

    QLabel *fileLabel = new QLabel("JSON Files:");
    mainLayout->addWidget(fileLabel);

    fileContainer = new QWidget();
    fileLayout = new QVBoxLayout(fileContainer);
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidget(fileContainer);
    scroll->setWidgetResizable(true);
    scroll->setMinimumHeight(200);
    mainLayout->addWidget(scroll);

    loadCities();
}

/**
 * @brief Destroys the dbWindow instance.
 */
dbWindow::~dbWindow()
{
    delete ui;
}

/**
 * @brief Loads available cities from the database directory.
 * @details Scans the database directory for subdirectories representing cities
 * and creates clickable buttons for each one.
 */
void dbWindow::loadCities()
{
    QString dbPath = db::getAppDataPath() + "/db";
    QDir dbDir(dbPath);
    QStringList cities = dbDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString &city : cities) {
        QPushButton *btn = new QPushButton(city);
        cityLayout->addWidget(btn);

        connect(btn, &QPushButton::clicked, this, [=]() {
            loadFilesForCity(city);
        });
    }
}

/**
 * @brief Loads JSON files for a specific city.
 * @param city Name of the city to load files for.
 * @details Clears previous file list and populates with new file buttons.
 */
void dbWindow::loadFilesForCity(const QString &city)
{
    // Clear previous file buttons
    QLayoutItem *child;
    while ((child = fileLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    QString cityPath = db::getAppDataPath() + "/db/" + city;
    QDir cityDir(cityPath);
    QStringList files = cityDir.entryList(QStringList() << "*.json", QDir::Files);

    for (const QString &file : files) {
        QPushButton *fileBtn = new QPushButton(file);
        QStringList parts = file.split('_');
        fileBtn->setText(QString("%1, from %2 @ %3%4 to %5 @ %6%7").arg(parts[0], parts[3], parts[4][0], parts[4][1], parts[1], parts[2][0], parts[2][1]));

        fileLayout->addWidget(fileBtn);
        connect(fileBtn, &QPushButton::clicked, this, [=]() {
            qDebug() << "Selected JSON file:" << cityPath + "/" + file;
            loadJsonFile(city, file);
        });
    }
}

/**
 * @brief Loads and processes a specific JSON file.
 * @param city City name where the file is located.
 * @param fileName Name of the JSON file to load.
 * @details Validates JSON format and sends data to MainWindow for processing.
 */
void dbWindow::loadJsonFile(const QString &city, const QString &fileName)
{
    QString filePath = db::getAppDataPath() + "/db/" + city + "/" + fileName;
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open file:" << filePath;
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (!doc.isObject()) {
        qWarning() << "Invalid JSON format.";
        return;
    }

    QJsonObject jsonObj = doc.object();
    qDebug() << "JSON Object loaded:" << jsonObj;

    m_mainWindow->handleLoadDb(jsonObj, city);
}
