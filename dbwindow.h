/**
 * @file dbwindow.h
 * @brief Database browser window for viewing saved sensor data.
 */

#ifndef DBWINDOW_H
#define DBWINDOW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QDir>
#include <QPushButton>
#include <QLabel>
#include <QDebug>

class MainWindow;

namespace Ui {
class dbWindow;
}

/**
 * @class dbWindow
 * @brief Provides a GUI interface for browsing and loading saved sensor data.
 *
 * The window displays:
 * - List of available cities (as directories)
 * - JSON files for selected city
 * - Allows loading data back into the main application
 */
class dbWindow : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the database browser window
     * @param mainWindow Pointer to the parent MainWindow instance
     * @param parent Parent widget (optional)
     */
    explicit dbWindow(MainWindow *mainWindow, QWidget *parent = nullptr);

    /**
     * @brief Destroys the dbWindow instance
     */
    ~dbWindow();

private:
    MainWindow *m_mainWindow; ///< Reference to parent main window
    Ui::dbWindow *ui; ///< UI components
    QVBoxLayout *fileLayout; ///< Layout for file buttons
    QVBoxLayout *cityLayout; ///< Layout for city buttons
    QWidget *fileContainer; ///< Container widget for scroll area

    /**
     * @brief Loads available cities from the database directory
     */
    void loadCities();

    /**
     * @brief Loads JSON files for a specific city
     * @param city Name of the city to load files for
     */
    void loadFilesForCity(const QString &city);

    /**
     * @brief Loads and processes a specific JSON file
     * @param city City name where the file is located
     * @param fileName Name of the JSON file to load
     */
    void loadJsonFile(const QString &city, const QString &fileName);
};

#endif // DBWINDOW_H
