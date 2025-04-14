/**
 * @file ApiClient.cpp
 * @brief Implementation of the ApiClient class for GIOS API communication.
 */

#include "ApiClient.h"
#include <QJsonArray>
#include <QJsonObject>

/**
 * @brief Constructs the ApiClient and initializes network manager.
 * @param parent Parent QObject (optional).
 */
ApiClient::ApiClient(QObject *parent) : QObject(parent) {
    manager = new QNetworkAccessManager(this);
}

/**
 * @brief Fetches all air quality monitoring stations from GIOS API.
 * @details Emits:
 * - `allStationsProcessed(QJsonArray)` on success.
 * - `errorOccurred(QString)` on network or data format errors.
 */
void ApiClient::getAllStations() {
    QUrl url(baseUrl + "/station/findAll");
    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        reply->deleteLater();

        if (doc.isNull() || !doc.isArray()) {
            emit errorOccurred("Invalid response format");
            return;
        }

        processStationsData(doc.array());
    });
}

/**
 * @brief Processes raw station data into a simplified JSON structure.
 * @param stations Raw QJsonArray from GIOS API.
 * @details Extracts: city, district, province, and street names.
 * Emits `allStationsProcessed(QJsonArray)` with filtered data.
 */
void ApiClient::processStationsData(const QJsonArray &stations) {
    QJsonArray filteredData;
    for (const QJsonValue &stationValue : stations) {
        QJsonObject station = stationValue.toObject();
        QJsonObject city = station["city"].toObject();
        QJsonObject commune = city["commune"].toObject();

        QJsonObject filteredStation;
        filteredStation["id"] = station["id"];
        filteredStation["city"] = commune["communeName"];
        filteredStation["district"] = commune["districtName"];
        filteredStation["province"] = commune["provinceName"];
        filteredStation["station_street"] = station["stationName"];

        filteredData.append(filteredStation);
    }
    emit allStationsProcessed(filteredData);
}

/**
 * @brief Fetches sensor details for a specific station.
 * @param stationId Unique ID of the station.
 * @details Emits:
 * - `stationDetailsReceived(QJsonObject)` on success.
 * - `errorOccurred(QString)` on failure.
 */
void ApiClient::getStationDetails(int stationId) {
    emit statusChanged(QString("Searching for sensors of station %1...").arg(stationId));
    QUrl url(baseUrl + QString("/station/sensors/%1").arg(stationId));
    QNetworkRequest request(url);

    auto reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        handleResponse(reply, [this](const QJsonDocument &doc) {
            if (doc.isObject()) {
                emit stationDetailsReceived(doc.object());
                emit statusChanged("Successfully retrieved station sensors");
            }
            else if (doc.isArray()) {
                QJsonObject wrapper;
                wrapper["data"] = doc.array();
                emit stationDetailsReceived(wrapper);
                emit statusChanged("Successfully retrieved station sensors");
            }
            else {
                emit errorOccurred("Invalid response format");
            }
        });
    });
}

/**
 * @brief Fetches measurement data for a specific sensor.
 * @param sensorId Unique ID of the sensor.
 * @details Emits:
 * - `sensorDataReceived(QJsonObject)` on success.
 * - `errorOccurred(QString)` on failure.
 */
void ApiClient::getSensorData(int sensorId) {
    emit statusChanged(QString("Searching for data of sensor %1...").arg(sensorId));
    QUrl url(baseUrl + QString("/data/getData/%1").arg(sensorId));
    QNetworkRequest request(url);

    auto reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        handleResponse(reply, [this](const QJsonDocument &doc) {
            if (doc.isObject()) {
                emit sensorDataReceived(doc.object());
                emit statusChanged("Successfully retrieved sensor data");
            }
            else if (doc.isArray()) {
                QJsonObject wrapper;
                wrapper["data"] = doc.array();
                emit sensorDataReceived(wrapper);
                emit statusChanged("Successfully retrieved sensor data");
            }
            else {
                emit errorOccurred("Invalid response format");
            }
        });
    });
}

/**
 * @brief Handles network responses and errors.
 * @param reply Network reply object.
 * @param successHandler Callback for processing successful JSON responses.
 * @details Emits `statusChanged(QString)` or `errorOccurred(QString)`.
 */
void ApiClient::handleResponse(QNetworkReply *reply,
                               std::function<void(const QJsonDocument&)> successHandler) {
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isNull()) {
            successHandler(doc);
            emit statusChanged("Success");
        } else {
            emit errorOccurred("Invalid JSON format");
        }
    } else {
        QString error = QString("Network error: %1").arg(reply->errorString());
        emit errorOccurred(error);
    }
    reply->deleteLater();
}
