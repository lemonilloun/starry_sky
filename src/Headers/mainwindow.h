#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include "StarCatalog.h"
#include "StarMapWidget.h"
#include "SettingsDialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_settingsButton_clicked();
    void buildStarMap(); // Построение карты звезд
    void onAnglesChanged();

private:
    Ui::MainWindow *ui;
    std::unique_ptr<StarCatalog> catalog; // Указатель на каталог звезд
    StarMapWidget *starMapWidget = nullptr; // Виджет карты звездного неба

    BlurParams  m_blurParams;
    FlareParams m_flareParams;

    bool        m_blurEnabled;
    bool        m_flareEnabled;
};

#endif // MAINWINDOW_H
