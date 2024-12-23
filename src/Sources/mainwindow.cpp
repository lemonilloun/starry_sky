#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "StarCatalog.h"
#include "StarMapWidget.h"

#include <QVBoxLayout>
#include <QMessageBox>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , catalog(std::make_unique<StarCatalog>("/Users/lehacho/starry_sky/Catalogue_clean.csv"))
{
    ui->setupUi(this);

    // Установка начальных значений полей
    ui->observerRaSpinBox->setValue(1.02703168676271);   // Восхождение
    ui->observerDecSpinBox->setValue(-0.00360223021825306); // Склонение
    ui->maxMagnitudeSpinBox->setValue(3.8);             // Звездная величина
    ui->fovXSpinBox->setValue(58.0);                    // Поле зрения X
    ui->fovYSpinBox->setValue(47.0);                    // Поле зрения Y

    // Подключаем кнопку построения карты
    connect(ui->buildMapButton, &QPushButton::clicked, this, &MainWindow::buildStarMap);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::buildStarMap() {
    // Получаем значения из полей ввода
    double observer_ra = ui->observerRaSpinBox->value();
    double observer_dec = ui->observerDecSpinBox->value();
    double maxMagnitude = ui->maxMagnitudeSpinBox->value();
    double fovX = ui->fovXSpinBox->value();
    double fovY = ui->fovYSpinBox->value();

    // Получаем звезды, которые видны наблюдателю
    auto visibleStars = catalog->getVisibleStars(observer_ra, observer_dec, maxMagnitude);

    // Преобразуем их в стереографическую проекцию
    auto [x, y] = catalog->stereographicProjection(visibleStars, fovX, fovY);

    std::vector<double> magnitudes;
    std::vector<double> colorIndices;
    magnitudes.reserve(visibleStars.size());
    colorIndices.reserve(visibleStars.size());

    for (const auto& star : visibleStars) {
        magnitudes.push_back(star.magnitude);
        colorIndices.push_back(star.colorIndex);
    }

    // Создаем или обновляем виджет карты звездного неба
    if (starMapWidget) {
        delete starMapWidget; // Удаляем старый виджет
    }

    starMapWidget = new StarMapWidget(x, y, magnitudes, colorIndices);
    starMapWidget->resize(1280, 1038);
    starMapWidget->show();
}
