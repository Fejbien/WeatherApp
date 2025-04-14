/**
 * @file db.cpp
 * @brief Implementation of database utility functions.
 */

#include "db.h"

// Initialize static member
QMap<QString, int> db::idMap;

/**
 * @brief Implementation of getAppDataPath().
 * @details Creates the following directory structure if it doesn't exist:
 *          - AppDataLocation/
 *          - AppDataLocation/db/
 */
QString db::getAppDataPath() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
        dir.mkpath("db");
    }
    return path;
}

/**
 * @brief Implementation of saveSensorData().
 * @details Saves data with filename format: [key]_[firstTimestamp]_[lastTimestamp].json
 * @warning Skips saving if:
 *          - Values array is empty
 *          - File already exists
 *          - File cannot be opened
 */
void db::saveSensorData(const QJsonObject &data, QString currentLocation) {
    QString key = data["key"].toString();
    QJsonArray values = data["values"].toArray();

    if (values.isEmpty()) {
        qWarning() << "No values in JSON data.";
        return;
    }

    // Sanitize timestamps for filename
    QString firstTimestamp = values.first().toObject()["date"].toString()
                                 .replace(":", "").replace(" ", "_");
    QString lastTimestamp = values.last().toObject()["date"].toString()
                                .replace(":", "").replace(" ", "_");

    // Ensure location directory exists
    QString locationPath = getAppDataPath() + "/db/" + currentLocation;
    QDir dir(locationPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString fileName = QString("%1/%2_%3_%4.json")
                           .arg(locationPath, key, firstTimestamp, lastTimestamp);

    if (QFile::exists(fileName)) {
        qWarning() << "File already exists:" << fileName;
        return;
    }

    QJsonDocument doc(data);
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open file for writing:" << fileName;
        return;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    qDebug() << "JSON saved to" << fileName;
}

/**
 * @brief Implementation of loadCityData().
 * @details Expected JSON format
 * @return QStringList Formatted as "City, District, Province, Street"
 * @warning Returns empty list if:
 *          - File cannot be opened
 *          - JSON format is invalid
 */
QStringList db::loadCityData(const QString &filePath) {
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
