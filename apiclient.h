#ifndef APICLIENT_H
#define APICLIENT_H

#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QJsonDocument>
#include <functional>

class ApiClient : public QObject {
    Q_OBJECT
public:
    explicit ApiClient(QObject *parent = nullptr);

    void getAllStations();
    void getStationDetails(int stationId);
    void getSensorData(int sensorId);
    bool checkNetConnection();

signals:
    void allStationsProcessed(const QJsonArray &filteredData);
    void stationDetailsReceived(const QJsonObject &details);
    void sensorDataReceived(const QJsonObject &data);
    void errorOccurred(const QString &error);
    void statusChanged(const QString &status);

private:
    QNetworkAccessManager *manager;
    const QString baseUrl = "https://api.gios.gov.pl/pjp-api/rest";

    void processStationsData(const QJsonArray &stations);
    void handleResponse(QNetworkReply *reply,
                        std::function<void(const QJsonDocument&)> successHandler);
};

#endif // APICLIENT_H
