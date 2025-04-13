#include "mainwindow.h"
#include "./ui_mainwindow.h"

QMap<QString, int> idMap;

QString getAppDataPath() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
        dir.mkpath("db");
    }

    return path;
}

QStringList loadCityData(const QString &filePath) {
    QStringList cityList;
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open file:" << filePath;
        return cityList;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (!doc.isArray()) {
        qWarning() << "Invalid JSON format.";
        return cityList;
    }

    QJsonArray jsonArray = doc.array();
    for (const QJsonValue &value : jsonArray) {
        if (!value.isObject()) continue;
        QJsonObject obj = value.toObject();

        int id = obj["id"].toInt();
        QString city = obj["city"].toString();
        QString district = obj["district"].toString();
        QString province = obj["province"].toString();
        QString stationStreet = obj["station_street"].toString();

        QString displayText = QString("%1, %2, %3, %4").arg(city, district, province, stationStreet);
        cityList.append(displayText);
        idMap[displayText] = id;
    }

    return cityList;
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // API
    apiClient = new ApiClient(this);
    // Get all stations
    connect(apiClient, &ApiClient::allStationsProcessed,
            this, &MainWindow::handleStationsData);

    // Get stations detail
    connect(apiClient, &ApiClient::stationDetailsReceived,
            this, &MainWindow::handleStationDetails);

    connect(apiClient, &ApiClient::sensorDataReceived,
            this, &MainWindow::handleSensorData);

    connect(apiClient, &ApiClient::statusChanged,
            this, &MainWindow::handleStatusChanged);

    connect(apiClient, &ApiClient::errorOccurred,
            this, &MainWindow::handleApiError);

    // Check the internet
    if(apiClient->checkNetConnection()){
        // good
    }
    else{
        // bad
    }

    // Get stations json
    QFile file(getAppDataPath() + "/citySearchData.json");
    if (file.exists()) {
        makeAutoComplete();
    }
    else{
        apiClient->getAllStations();
    }
}

void MainWindow::makeAutoComplete(){
    QStringList cityList = loadCityData(getAppDataPath() + "/citySearchData.json");
    QCompleter *completer = new QCompleter(cityList);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    ui->cityInput->setCompleter(completer);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete apiClient;
}

void MainWindow::onCitySearchClicked()
{
    QString city = ui->cityInput->text();
    int id = idMap[city];

    if(id == 0){
        ui->resultBrowser->setText("Wybierz poprawna nazwe!");
        return;
    }

    apiClient->getStationDetails(id);
}
void MainWindow::handleStationsData(const QJsonArray &data)
{
    QFile file(getAppDataPath() + "/citySearchData.json");
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }

    QJsonDocument doc(data);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    makeAutoComplete();
}

void MainWindow::handleStationDetails(const QJsonObject &details)
{
    qDebug() << "Raw station details:" << details;

    clearSensorButtons();

    // Check if "data" field exists
    if (!details.contains("data")) {
        qDebug() << "No 'data' field in response";
        return;
    }

    QJsonArray sensorsData = details["data"].toArray();
    if (sensorsData.isEmpty()) {
        qDebug() << "Empty sensors data array";
        return;
    }

    QWidget *container = ui->sensorScrollArea->widget();
    if (!container) {
        qDebug() << "Scroll area container not found";
        container = new QWidget();
        ui->sensorScrollArea->setWidget(container);
        container->setLayout(new QVBoxLayout());
    }

    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(container->layout());
    if (!layout) {
        layout = new QVBoxLayout(container);
        layout->setAlignment(Qt::AlignTop);
    }

    for (const QJsonValue &sensorValue : sensorsData) {
        QJsonObject sensor = sensorValue.toObject();

        // Access the nested "param" object
        QJsonObject param = sensor["param"].toObject();

        QString paramName = param["paramName"].toString();
        QString paramCode = param["paramCode"].toString();
        int sensorId = sensor["id"].toInt();  // Use this ID for requests

        qDebug() << "Creating button for:" << paramName << "(" << paramCode << ") ID:" << sensorId;

        QPushButton *btn = new QPushButton(
            QString("%1 (%2)").arg(paramName).arg(paramCode),
            container
            );

        // Store both the display code and the actual ID
        btn->setProperty("paramCode", paramCode);
        btn->setProperty("sensorId", sensorId);

        // Style the button
        btn->setMinimumHeight(40);
        btn->setStyleSheet(
            "QPushButton {"
            "   background-color: #4CAF50;"
            "   color: white;"
            "   padding: 8px;"
            "   border-radius: 4px;"
            "   margin: 2px;"
            "}"
            "QPushButton:hover { background-color: #45a049; }"
            );

        connect(btn, &QPushButton::clicked, this, [this, btn]() {
            int sensorId = btn->property("sensorId").toInt();
            qDebug() << "Requesting data for sensor ID:" << sensorId;
            apiClient->getSensorData(sensorId);

            // Highlight button
            for (QPushButton *otherBtn : sensorButtons) {
                otherBtn->setStyleSheet(
                    otherBtn == btn ?
                        "background-color: #45a049;" :
                        "background-color: #4CAF50;"
                    );
            }
        });

        layout->addWidget(btn);
        sensorButtons.append(btn);
    }

    // Ensure proper layout update
    container->adjustSize();
    qDebug() << "Created" << sensorButtons.size() << "sensor buttons";
}

void MainWindow::clearSensorButtons()
{
    // First clear all buttons from memory
    qDeleteAll(sensorButtons);
    sensorButtons.clear();

    // Then clear them from the layout
    QWidget *container = ui->sensorScrollArea->widget();
    if (!container) return;

    QLayout *layout = container->layout();
    if (layout) {
        QLayoutItem *item;
        while ((item = layout->takeAt(0)) != nullptr) {  // Fixed while condition
            if (item->widget()) {
                delete item->widget();
            }
            delete item;
        }
    }
}

void MainWindow::handleSensorData(const QJsonObject &data)
{
    qDebug() << "Sensor data received:" << data;

    // Clear previous content
    QWidget *oldWidget = ui->resultScrollArea->takeWidget();
    if (oldWidget) {
        delete oldWidget;
    }

    if (!data.contains("values")) {
        qWarning() << "No 'values' in sensor data";
        return;
    }

    QJsonArray values = data["values"].toArray();
    if (values.isEmpty()) {
        qWarning() << "Empty values array";
        return;
    }

    // Create chart and series
    QChart *chart = new QChart();
    QLineSeries *series = new QLineSeries();

    QString paramName = data["key"].toString();
    double minValue = std::numeric_limits<double>::max();
    double maxValue = std::numeric_limits<double>::min();
    double sum = 0;
    int validCount = 0;

    // Parse and sort data by date (newest first)
    QVector<QPair<QDateTime, double>> measurements;
    for (const QJsonValue &value : values) {
        QJsonObject measurement = value.toObject();
        QString valueStr = measurement["value"].toString();
        if (valueStr == "null") continue;

        QDateTime dateTime = QDateTime::fromString(measurement["date"].toString(), "yyyy-MM-dd HH:mm:ss");
        if (!dateTime.isValid()) {
            qWarning() << "Invalid date format:" << measurement["date"].toString();
            continue;
        }

        double val = valueStr.toDouble();
        measurements.append(qMakePair(dateTime, val));
    }

    // Sort measurements by date (oldest first)
    std::sort(measurements.begin(), measurements.end(),
              [](const QPair<QDateTime, double> &a, const QPair<QDateTime, double> &b) {
                  return a.first < b.first;
              });

    // Add points to series and calculate stats
    for (const auto &measurement : measurements) {
        series->append(measurement.first.toMSecsSinceEpoch(), measurement.second);

        if (measurement.second < minValue) minValue = measurement.second;
        if (measurement.second > maxValue) maxValue = measurement.second;
        sum += measurement.second;
        validCount++;
    }

    if (validCount == 0) {
        qWarning() << "No valid measurements found";
        return;
    }

    double avgValue = sum / validCount;

    // Configure chart
    chart->addSeries(series);
    chart->setTitle(QString("%1 Measurements").arg(paramName));
    chart->legend()->hide();

    // X-axis (time)
    QDateTimeAxis *axisX = new QDateTimeAxis();
    axisX->setFormat("dd.MM.yy\nhh:mm");
    axisX->setTitleText("Time");
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // Y-axis (values)
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText(paramName);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // Create chart view
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    // Create container with stats
    QWidget *container = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(container);

    // Add stats label
    QLabel *statsLabel = new QLabel(
        QString("<b>Statistics for %1:</b><br>"
                "Minimum: %2<br>"
                "Maximum: %3<br>"
                "Average: %4<br>"
                "Measurements: %5")
            .arg(paramName)
            .arg(minValue, 0, 'f', 1)
            .arg(maxValue, 0, 'f', 1)
            .arg(avgValue, 0, 'f', 1)
            .arg(validCount));
    statsLabel->setTextFormat(Qt::RichText);

    layout->addWidget(statsLabel);
    layout->addWidget(chartView);

    // Set to scroll area
    ui->resultScrollArea->setWidget(container);
    container->adjustSize();

    // After adding all points to the series
    qDebug() << "Number of points in series:" << series->count();
    if (series->count() > 0) {
        qDebug() << "First point:" << series->at(0);
        qDebug() << "Last point:" << series->at(series->count()-1);
    }

    // After creating axes
    qDebug() << "X axis range:" << axisX->min() << "to" << axisX->max();
    qDebug() << "Y axis range:" << axisY->min() << "to" << axisY->max();
}

void MainWindow::handleApiError(const QString &error)
{
    qDebug() << error;
    ui->resultBrowser->setText(error);
}

void MainWindow::handleStatusChanged(const QString &status)
{
    qDebug() << status;
    ui->resultBrowser->setText(status);
}


void MainWindow::on_citySearch_clicked()
{
    onCitySearchClicked();
}

