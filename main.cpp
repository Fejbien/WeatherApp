/**
 * @file main.cpp
 * @brief Main application entry point for the Air Quality Monitoring System.
 *
 * This file contains the main() function which initializes the Qt application
 * and creates the main window.
 */

#include "mainwindow.h"
#include <QApplication>

/**
 * @brief Application entry point.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return int Application exit code.
 *
 * @details Initializes the Qt application and creates the main window:
 * 1. Creates QApplication instance (required for any Qt GUI application)
 * 2. Creates and shows the MainWindow
 * 3. Enters the main event loop
 *
 * @note The QApplication object must be created before any Qt GUI components.
 */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);  ///< Main Qt application object
    MainWindow w;                ///< Main application window
    w.show();                    ///< Display the main window
    return a.exec();             ///< Enter main event loop
}
