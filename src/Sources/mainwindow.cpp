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
    , catalog(std::make_unique<StarCatalog>("/Users/lehacho/starry_sky/src/data/Catalogue_clean.csv"))
{
    ui->setupUi(this);

    if (!ui->MapWidget->layout()) {
        QVBoxLayout* layout = new QVBoxLayout(ui->MapWidget);
        layout->setContentsMargins(0, 0, 0, 0);
        ui->MapWidget->setLayout(layout);
    }

    // --- Создаём QLabel с заглушкой ---
    QLabel* placeholder = new QLabel(ui->MapWidget);
    placeholder->setScaledContents(true);
    placeholder->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    QPixmap pix("/Users/lehacho/starry_sky/src/data/placeholder.png");
    placeholder->setPixmap(pix);

    // Добавляем QLabel в лэйаут
    ui->MapWidget->layout()->addWidget(placeholder);

    // Установка начальных значений полей
    ui->observerRaSpinBox->setValue(1.02703168676271);   // Восхождение
    ui->observerDecSpinBox->setValue(-0.00360223021825306); // Склонение
    ui->maxMagnitudeSpinBox->setValue(3.8);             // Звездная величина
    ui->fovXSpinBox->setValue(58.0);                    // Поле зрения X
    ui->fovYSpinBox->setValue(47.0);                    // Поле зрения Y
    ui->thetaSpinBox->setValue(0.0);
    ui->psiSpinBox->setValue(0.0);
    ui->phiSpinBox->setValue(0.0);

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
    double theta_deg = ui->thetaSpinBox->value();
    double psi_deg = ui->psiSpinBox->value();
    double phi_deg = ui->phiSpinBox->value();

    // Получаем звезды, которые видны наблюдателю
    auto visibleStars = catalog->getVisibleStars(observer_ra, observer_dec, maxMagnitude);

    // Преобразуем их в стереографическую проекцию
    auto [x, y] = catalog->stereographicProjection(visibleStars, fovX, fovY, theta_deg, psi_deg, phi_deg);

    std::vector<double> magnitudes;
    std::vector<double> colorIndices;
    magnitudes.reserve(visibleStars.size());
    colorIndices.reserve(visibleStars.size());

    for (const auto& star : visibleStars) {
        magnitudes.push_back(star.magnitude);
        colorIndices.push_back(star.colorIndex);
    }

    // 5) Удаляем старый виджет (если ранее создавали)
    //    Чтобы не копить их, очищаем лэйаут MapWidget:
    QLayout *mapLayout = ui->MapWidget->layout();
    if (mapLayout) {
        QLayoutItem *child;
        while ((child = mapLayout->takeAt(0)) != nullptr) {
            if (child->widget()) {
                delete child->widget(); // Удаляем старый StarMapWidget
            }
            delete child;
        }
    }

    // 6) Создаём новый StarMapWidget с нужными данными
    //    Родитель = ui->MapWidget, чтобы он жил внутри.
    StarMapWidget* mapWidget = new StarMapWidget(x, y, magnitudes, colorIndices,
                                                 ui->MapWidget);

    // 7) Добавляем его в лэйаут
    mapLayout->addWidget(mapWidget);
}
