#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qpushbutton.h>
#include <qjsondocument.h>
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QCompleter>
#include <QStringListModel>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QtCharts>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QDateTimeAxis>
#include <QtMath>
#include "./apiClient.h"
#include "./db.h"
#include "./dbwindow.h"


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    void handleLoadDb(const QJsonObject &data, QString location);
    ~MainWindow();

private slots:
    void onCitySearchClicked();
    void handleStationsData(const QJsonArray &data);
    void handleStationDetails(const QJsonObject &details);
    void handleSensorData(const QJsonObject &data);
    void handleApiError(const QString &error);
    void handleStatusChanged(const QString &status);

    void on_citySearch_clicked(); // Auto grenerated by QT when trying to change
    void on_dbOpener_clicked(); // snake case to camel error poped up :/

private:
    Ui::MainWindow *ui;
    QVector<QPushButton*> sensorButtons;
    ApiClient *apiClient;
    QChartView *chartView = nullptr;
    QString currentLocation;
    db dbAccess;
    bool isFromInternet;

    void makeAutoComplete();
    void clearSensorButtons();
};
#endif // MAINWINDOW_H
