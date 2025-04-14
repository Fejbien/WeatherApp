/**
 * @file ApiClient.h
 * @brief API client for communicating with GIOS (Polish Environmental Protection Inspectorate) air quality data service.
 */

#ifndef APICLIENT_H
#define APICLIENT_H

#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QJsonDocument>
#include <functional>

/**
 * @class ApiClient
 * @brief Provides interface to GIOS REST API for air quality data.
 *
 * This class handles network communication and data processing for:
 * - Retrieving list of monitoring stations
 * - Fetching station details
 * - Getting sensor measurements
 */
class ApiClient : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Constructs an ApiClient instance
     * @param parent Parent QObject (optional)
     */
    explicit ApiClient(QObject *parent = nullptr);

    /**
     * @brief Requests list of all air quality monitoring stations
     * @note Emits allStationsProcessed() or errorOccurred() when complete
     */
    void getAllStations();

    /**
     * @brief Requests details for specific monitoring station
     * @param stationId Unique identifier of the station
     * @note Emits stationDetailsReceived() or errorOccurred() when complete
     */
    void getStationDetails(int stationId);

    /**
     * @brief Requests measurement data from specific sensor
     * @param sensorId Unique identifier of the sensor
     * @note Emits sensorDataReceived() or errorOccurred() when complete
     */
    void getSensorData(int sensorId);

signals:
    /**
     * @brief Emitted when station list data is processed and ready
     * @param filteredData Processed station data in simplified format
     */
    void allStationsProcessed(const QJsonArray &filteredData);

    /**
     * @brief Emitted when station details are received
     * @param details JSON object containing station sensors information
     */
    void stationDetailsReceived(const QJsonObject &details);

    /**
     * @brief Emitted when sensor measurement data is received
     * @param data JSON object containing sensor readings
     */
    void sensorDataReceived(const QJsonObject &data);

    /**
     * @brief Emitted when API request fails
     * @param error Description of the error that occurred
     */
    void errorOccurred(const QString &error);

    /**
     * @brief Emitted to report status changes during operations
     * @param status Current operation status message
     */
    void statusChanged(const QString &status);

private:
    QNetworkAccessManager *manager; ///< Handles network communication
    const QString baseUrl = "https://api.gios.gov.pl/pjp-api/rest"; ///< GIOS API base URL

    /**
     * @brief Processes raw station data into simplified format
     * @param stations Raw JSON array of station data from API
     */
    void processStationsData(const QJsonArray &stations);

    /**
     * @brief Handles network response and error processing
     * @param reply Network reply object
     * @param successHandler Callback function for successful responses
     */
    void handleResponse(QNetworkReply *reply,
                        std::function<void(const QJsonDocument&)> successHandler);
};

#endif // APICLIENT_H
