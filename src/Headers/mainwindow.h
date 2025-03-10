#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include "StarCatalog.h"
#include "StarMapWidget.h"

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
    void buildStarMap(); // Построение карты звезд

private:
    Ui::MainWindow *ui;
    std::unique_ptr<StarCatalog> catalog; // Указатель на каталог звезд
    StarMapWidget *starMapWidget = nullptr; // Виджет карты звездного неба
};

#endif // MAINWINDOW_H
