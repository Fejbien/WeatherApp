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
#include "./apiClient.h"

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
    ~MainWindow();

private slots:
    void onCitySearchClicked();
    void handleStationsData(const QJsonArray &data);
    void handleStationDetails(const QJsonObject &details);
    void handleSensorData(const QJsonObject &data);
    void handleApiError(const QString &error);
    void handleStatusChanged(const QString &status);

    void on_citySearch_clicked();

private:
    Ui::MainWindow *ui;
    QVector<QPushButton*> sensorButtons;
    ApiClient *apiClient;
    QChartView *chartView = nullptr;

    void makeAutoComplete();
    void clearSensorButtons();
};
#endif // MAINWINDOW_H
