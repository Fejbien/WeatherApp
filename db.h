/**
 * @file db.h
 * @brief Database utility class for handling application data storage and retrieval.
 */

#ifndef DB_H
#define DB_H

#include <qobject.h>
#include <qstandardpaths.h>
#include <qdir.h>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QTextStream>
#include <QJsonArray>

/**
 * @class db
 * @brief Provides static methods for data persistence operations.
 *
 * This class handles:
 * - Application data directory management
 * - JSON data storage for sensor readings
 * - City data loading and mapping
 */
class db
{
public:
    /**
     * @brief Gets the application data storage path.
     * @return QString Absolute path to application data directory.
     * @note Creates the directory structure if it doesn't exist.
     */
    static QString getAppDataPath();

    /**
     * @brief Loads city data from a JSON file.
     * @param filePath Path to the JSON file containing city data.
     * @return QStringList List of formatted city entries ("City, District, Province, Street").
     * @note Populates the idMap with corresponding IDs for each entry.
     */
    static QStringList loadCityData(const QString &filePath);

    /**
     * @brief Saves sensor data to a JSON file.
     * @param data QJsonObject containing sensor readings.
     * @param currentLocation Location identifier for file organization.
     * @note Files are saved in AppDataLocation/db/[location]/ with timestamped filenames.
     */
    static void saveSensorData(const QJsonObject &data, QString currentLocation);

    /**
     * @brief Mapping between city display strings and their IDs.
     * @note Populated by loadCityData().
     */
    static QMap<QString, int> idMap;
};

#endif // DB_H
