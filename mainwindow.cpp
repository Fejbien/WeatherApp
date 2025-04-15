/**
 * @file mainwindow.cpp
 * @brief Implementation of the MainWindow class.
 */

#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow), isFromInternet(false) // Initialize data source flag
{
    ui->setupUi(this);
    // Initialize API client and connect signals
    apiClient = new ApiClient(this);
    connect(apiClient, &ApiClient::allStationsProcessed, this, &MainWindow::handleStationsData);
    connect(apiClient, &ApiClient::stationDetailsReceived, this, &MainWindow::handleStationDetails);
    connect(apiClient, &ApiClient::sensorDataReceived, this, &MainWindow::handleSensorData);
    connect(apiClient, &ApiClient::statusChanged, this, &MainWindow::handleStatusChanged);
    connect(apiClient, &ApiClient::errorOccurred, this, &MainWindow::handleApiError);

    // Try to load cached city data first
    QString cachePath = dbAccess.getAppDataPath() + "/citySearchData.json";
    if (QFile::exists(cachePath))
        makeAutoComplete();
    else
        apiClient->getAllStations(); // Fetch fresh data if no cache exists
}

MainWindow::~MainWindow()
{
    delete ui;
    delete apiClient;
}

/**
 * @brief Initializes city search autocomplete
 */
void MainWindow::makeAutoComplete()
{
    QString cachePath = dbAccess.getAppDataPath() + "/citySearchData.json";
    QStringList cityList = dbAccess.loadCityData(cachePath);

    QCompleter *completer = new QCompleter(cityList, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    ui->cityInput->setCompleter(completer);
}

/**
 * @brief Handles city search button click event
 */
void MainWindow::onCitySearchClicked()
{
    QString city = ui->cityInput->text();

    // Validate city selection
    if (!dbAccess.idMap.contains(city) || dbAccess.idMap[city] == 0) {
        ui->resultBrowser->setText("Please select a valid city name!");
        return;
    }

    currentLocation = city;
    apiClient->getStationDetails(dbAccess.idMap[city]);
}

/**
 * @brief Processes station list data and caches it locally
 * @param data JSON array containing station data
 */
void MainWindow::handleStationsData(const QJsonArray &data)
{
    QFile file(dbAccess.getAppDataPath() + "/citySearchData.json"); // Saves
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(data).toJson(QJsonDocument::Indented));
        file.close();
    }

    makeAutoComplete();
}

/**
 * @brief Processes station details and creates sensor buttons
 * @param details JSON object containing sensor information
 */
void MainWindow::handleStationDetails(const QJsonObject &details)
{
    clearSensorButtons();

    if (!details.contains("data")) {
        qDebug() << "Invalid station details format";
        return;
    }

    QJsonArray sensorsData = details["data"].toArray();
    if (sensorsData.isEmpty()) {
        qDebug() << "No sensors found for this station";
        return;
    }

    // Create layout for sensor buttons
    QWidget *container = ui->sensorScrollArea->widget();
    QVBoxLayout *layout = container->layout() ? qobject_cast<QVBoxLayout*>(container->layout()) : new QVBoxLayout(container);
    layout->setAlignment(Qt::AlignTop);

    // Create button for each sensor
    for (const QJsonValue &sensorValue : sensorsData) {
        QJsonObject sensor = sensorValue.toObject();
        QJsonObject param = sensor["param"].toObject();

        QString paramName = param["paramName"].toString();
        QString paramCode = param["paramCode"].toString();
        int sensorId = sensor["id"].toInt();

        QPushButton *btn = new QPushButton(QString("%1 (%2)").arg(paramName, paramCode), container);

        // Store sensor metadata in button properties
        btn->setProperty("paramCode", paramCode);
        btn->setProperty("sensorId", sensorId);
        btn->setMinimumHeight(40);

        // Connect button to data fetch
        connect(btn, &QPushButton::clicked, this, [this, btn]() {
            isFromInternet = true;
            apiClient->getSensorData(btn->property("sensorId").toInt());
        });

        layout->addWidget(btn);
        sensorButtons.append(btn);
    }

    container->adjustSize();
}

/**
 * @brief Clears all sensor buttons from the UI and memory.
 * @details Removes buttons both from the layout and deallocates memory.
 */
void MainWindow::clearSensorButtons()
{
    qDeleteAll(sensorButtons);
    sensorButtons.clear();

    QWidget *container = ui->sensorScrollArea->widget();
    if (!container) return;

    QLayout *layout = container->layout();
    if (layout) {
        QLayoutItem *item;
        while ((item = layout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                delete item->widget();
            }
            delete item;
        }
    }
}

/**
 * @brief Processes and visualizes sensor data.
 * @param data JSON object containing sensor measurements.
 * @details Creates an interactive chart with time range sliders and statistics.
 */
void MainWindow::handleSensorData(const QJsonObject &data)
{
    // Clear previous visualization
    QWidget *oldWidget = ui->resultScrollArea->takeWidget();
    delete oldWidget;

    if (!data.contains("values")) {
        qWarning() << "Missing values in sensor data";
        return;
    }

    QJsonArray values = data["values"].toArray();
    if (values.isEmpty()) {
        qWarning() << "Empty values array";
        return;
    }

    // Initialize chart components
    QChart *chart = new QChart();
    QLineSeries *series = new QLineSeries();
    QLineSeries *fullSeries = new QLineSeries();
    QString paramName = data["key"].toString();

    // Initialize statistics tracking
    double minValue = 99999;
    double maxValue = -99999;
    double sum = 0;
    int validCount = 0;
    QDateTime minDate, maxDate;

    // Process each measurement point
    for (const QJsonValue &value : values) {
        QJsonObject measurement = value.toObject();

        // Parse values
        double val = 0;
        bool validValue = false;
        QJsonValue valueJson = measurement["value"];

        if (valueJson.isString()) {
            QString valueStr = valueJson.toString();
            if (valueStr != "null") {
                val = valueStr.toDouble(&validValue);
            }
        } else if (valueJson.isDouble()) {
            val = valueJson.toDouble();
            validValue = true;
        }

        if (!validValue) continue;

        // Parse timestamp
        QDateTime dateTime = QDateTime::fromString(measurement["date"].toString(), "yyyy-MM-dd HH:mm:ss");
        if (!dateTime.isValid()) continue;

        // Add point to series
        QPointF point(dateTime.toMSecsSinceEpoch(), val);
        series->append(point);
        fullSeries->append(point);

        // Update statistics
        minValue = qMin(minValue, val);
        maxValue = qMax(maxValue, val);
        sum += val;
        validCount++;

        // Update date range
        if (!minDate.isValid() || dateTime < minDate) minDate = dateTime;
        if (!maxDate.isValid() || dateTime > maxDate) maxDate = dateTime;
    }

    if (validCount == 0) {
        qWarning() << "No valid measurements found";
        return;
    }

    // Create time range sliders
    QWidget *sliderContainer = new QWidget();
    QVBoxLayout *sliderLayout = new QVBoxLayout(sliderContainer);

    QLabel *sliderLabel = new QLabel("Select time range:");
    QSlider *startSlider = new QSlider(Qt::Horizontal);
    QSlider *endSlider = new QSlider(Qt::Horizontal);

    startSlider->setRange(0, 100);
    endSlider->setRange(0, 100);
    startSlider->setValue(0);
    endSlider->setValue(100);

    sliderLayout->addWidget(sliderLabel);
    sliderLayout->addWidget(startSlider);
    sliderLayout->addWidget(endSlider);

    // Configure chart axes
    QDateTimeAxis *axisX = new QDateTimeAxis();
    axisX->setFormat("dd.MM HH:mm");
    axisX->setTitleText("Time");
    axisX->setRange(minDate, maxDate);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText(paramName + " (µg/m³)");
    axisY->setRange(minValue > 0 ? 0 : minValue * 1.1, maxValue * 1.1);

    // Assemble chart
    chart->addSeries(series);
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisX);
    series->attachAxis(axisY);
    chart->legend()->hide();
    chart->setTitle(paramName + " Measurements");
    chart->setAnimationOptions(QChart::SeriesAnimations);

    // Create chart view
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    // Create main container
    QWidget *container = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(container);
    QLabel *statsLabel = new QLabel();
    layout->addWidget(statsLabel);

    /**
     * @brief Updates chart and statistics based on current slider positions.
     */
    auto updateChart = [=]() {
        int startPercent = startSlider->value();
        int endPercent = endSlider->value();

        // Validate slider positions
        if (startPercent > endPercent) return;

        // Calculate time range
        qint64 totalSpan = minDate.secsTo(maxDate);
        QDateTime startTime = minDate.addSecs(totalSpan * startPercent / 100);
        QDateTime endTime = minDate.addSecs(totalSpan * endPercent / 100);

        // Filter points within time range
        QLineSeries *filtered = new QLineSeries();
        for (const QPointF &point : fullSeries->points()) {
            QDateTime t = QDateTime::fromMSecsSinceEpoch(point.x());
            if (t >= startTime && t <= endTime) {
                filtered->append(point);
            }
        }

        // Calculate filtered statistics
        QString trendText = "Not enough data";
        double filteredMin = 99999;
        double filteredMax = -99999;
        double filteredSum = 0;
        int filteredCount = filtered->count();

        if (filteredCount >= 1) {
            for (const QPointF &point : filtered->points()) {
                double y = point.y();
                filteredMin = qMin(filteredMin, y);
                filteredMax = qMax(filteredMax, y);
                filteredSum += y;
            }

            // Determine trend if enough points
            if (filteredCount >= 2) {
                double first = filtered->at(0).y();
                double last = filtered->at(filteredCount - 1).y();
                double delta = last - first;

                if (qAbs(delta) < 0.1 * qAbs(first))
                    trendText = "Stable";
                else
                    trendText = (delta > 0) ? "Falling trend" : "Rising trend";
            }
        }

        // Update statistics display
        statsLabel->setText(
            QString("<b>Location:</b> %1<br>"
                    "<b>Parameter:</b> %2<br>"
                    "<b>Minimum:</b> %3 µg/m³<br>"
                    "<b>Maximum:</b> %4 µg/m³<br>"
                    "<b>Average:</b> %5 µg/m³<br>"
                    "<b>Measurements:</b> %6<br>"
                    "<b>Trend:</b> %7")
                .arg(currentLocation.isEmpty() ? "Unknown" : currentLocation)
                .arg(paramName)
                .arg(filteredMin, 0, 'f', 1)
                .arg(filteredMax, 0, 'f', 1)
                .arg(filteredCount > 0 ? filteredSum / filteredCount : 0.0, 0, 'f', 1)
                .arg(filteredCount).arg(trendText)
            );

        // Update chart display
        chart->removeAllSeries();
        chart->addSeries(filtered);
        filtered->attachAxis(axisX);
        filtered->attachAxis(axisY);
    };

    // Connect slider signals
    connect(startSlider, &QSlider::valueChanged, this, [=]() {
        if (startSlider->value() > endSlider->value())
            startSlider->setValue(endSlider->value());
        updateChart();
    });

    connect(endSlider, &QSlider::valueChanged, this, [=]() {
        if (endSlider->value() < startSlider->value())
            endSlider->setValue(startSlider->value());
        updateChart();
    });

    // Initial update
    updateChart();

    // Assemble UI components
    layout->addWidget(sliderContainer);
    layout->addWidget(statsLabel);
    layout->addWidget(chartView);
    layout->setStretch(2, 1); // Give chart most space

    // Add save button if from the internet
    if (isFromInternet) {
        QPushButton *saveButton = new QPushButton("Save to local DB");
        layout->addWidget(saveButton);

        connect(saveButton, &QPushButton::clicked, this, [=]() {
            dbAccess.saveSensorData(data, currentLocation);
            QMessageBox::information(this, "Saved", "Data has been saved to local database.");
        });
    }

    // Display the complete widget
    ui->resultScrollArea->setWidget(container);
    container->adjustSize();
}

/**
 * @brief Handles API error messages.
 * @param error Error message to display.
 */
void MainWindow::handleApiError(const QString &error)
{
    ui->resultBrowser->setText(error + " Try again! Or check already downloaded data.");
}

/**
 * @brief Updates status messages in the UI.
 * @param status Status message to display.
 */
void MainWindow::handleStatusChanged(const QString &status)
{
    ui->resultBrowser->setText(status);
}

/**
 * @brief Slot for city search button (auto-connected by Qt).
 */
void MainWindow::on_citySearch_clicked()
{
    onCitySearchClicked(); // Forward to main implementation
}

/**
 * @brief Opens the database browser window.
 */
void MainWindow::on_dbOpener_clicked()
{
    dbWindow *dbWin = new dbWindow(this);
    dbWin->setWindowFlag(Qt::Window, true);
    dbWin->show();
}

/**
 * @brief Handles loading data from local database.
 * @param data JSON data loaded from file.
 * @param location Location name associated with the data.
 */
void MainWindow::handleLoadDb(const QJsonObject &data, QString location)
{
    isFromInternet = false; // Mark as local data source
    ui->resultBrowser->setText("Loaded from local database");
    currentLocation = location;
    handleSensorData(data); // Process like API data
}
