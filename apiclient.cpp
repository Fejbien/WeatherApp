#include "ApiClient.h"
#include <qjsonarray.h>
#include <qjsonobject.h>

ApiClient::ApiClient(QObject *parent) : QObject(parent)
{
    manager = new QNetworkAccessManager(this);
}

void ApiClient::getAllStations()
{
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
            emit errorOccurred("zly format");
            return;
        }

        processStationsData(doc.array());
    });
}

void ApiClient::processStationsData(const QJsonArray &stations)
{
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

void ApiClient::getStationDetails(int stationId)
{
    emit statusChanged(QString("Wyszukiwanie sensorow stacji %1...").arg(stationId));
    QUrl url(baseUrl + QString("/station/sensors/%1").arg(stationId));
    QNetworkRequest request(url);
    qDebug() << url;

    auto reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        handleResponse(reply, [this](const QJsonDocument &doc) {
            if (doc.isObject()) {
                emit stationDetailsReceived(doc.object());
                emit statusChanged("Pomyslnie wyszukano sensory stacji");
            }
            else if (doc.isArray()) {
                QJsonObject wrapper;
                wrapper["data"] = doc.array();
                emit stationDetailsReceived(wrapper);
                emit statusChanged("Pomyslnie wyszukano sensory stacji");
            }
            else {
                emit errorOccurred("zly format");
            }
        });
    });
}

void ApiClient::getSensorData(int sensorId)
{
    emit statusChanged(QString("Wyszukiwanie danych sensora %1...").arg(sensorId));
    QUrl url(baseUrl + QString("/data/getData/%1").arg(sensorId));
    QNetworkRequest request(url);

    auto reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        handleResponse(reply, [this](const QJsonDocument &doc) {
            if (doc.isObject()) {
                emit sensorDataReceived(doc.object());
                emit statusChanged("Pomyslnie wyszukano dane sensora");
            }
            else if (doc.isArray()) {
                QJsonObject wrapper;
                wrapper["data"] = doc.array();
                emit sensorDataReceived(wrapper);
                emit statusChanged("Pomyslnie wyszukano dane sensora");
            }
            else {
                emit errorOccurred("zly format");
            }
        });
    });
}

void ApiClient::handleResponse(QNetworkReply *reply,
                               std::function<void(const QJsonDocument&)> successHandler)
{
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isNull()) {
            successHandler(doc);
            emit statusChanged("Sukces");
        } else {
            emit errorOccurred("Blad");
        }
    } else {
        QString error = QString("Error sieci: %1").arg(reply->errorString());
        emit errorOccurred(error);
    }
    reply->deleteLater();
}

bool ApiClient::checkNetConnection()
{
    bool isConnected = false;
    QTcpSocket сonnectionSocket;
    сonnectionSocket.connectToHost("google.com", 80);
    сonnectionSocket.waitForConnected(4000);

    if (сonnectionSocket.state() == QTcpSocket::ConnectedState) {
        isConnected = true;
    }

    сonnectionSocket.close();
    return isConnected;
}
