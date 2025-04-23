#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "StarCatalog.h"
#include "StarMapWidget.h"

#include <QVBoxLayout>
#include <QMessageBox>
#include <QDebug>
#include <cmath>

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

    // --- QLabel с заглушкой ---
    QLabel* placeholder = new QLabel(ui->MapWidget);
    placeholder->setScaledContents(true);
    placeholder->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    QPixmap pix("/Users/lehacho/starry_sky/src/data/placeholder3.png");
    placeholder->setPixmap(pix);

    ui->MapWidget->layout()->addWidget(placeholder);

    //ui->GLWidget->setAngles(0, 0, 0);

    ui->observerRaSpinBox->setValue(1.02703168676271);   // RA
    ui->observerDecSpinBox->setValue(-0.00360223021825306); // Dec
    ui->beta1SpinBox->setValue(0.0);
    ui->beta2SpinBox->setValue(0.0);
    ui->pSpinBox->setValue(0.0);

    ui->maxMagnitudeSpinBox->setValue(4.5);
    ui->fovXSpinBox->setValue(20.0);
    ui->fovYSpinBox->setValue(20.0);


    connect(ui->buildMapButton, &QPushButton::clicked,
            this, &MainWindow::buildStarMap);

    connect(ui->beta1SpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onAnglesChanged);
    connect(ui->beta2SpinBox,   QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onAnglesChanged);
    connect(ui->pSpinBox,   QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onAnglesChanged);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::buildStarMap()
{
    // 1) Считываем значения углов из UI
    double alpha0_deg = ui->observerRaSpinBox->value();   // RA визирования
    double dec0_deg   = ui->observerDecSpinBox->value();  // Dec визирования
    double beta1_deg  = ui->beta1SpinBox->value();        // угол вокруг xi
    double beta2_deg  = ui->beta2SpinBox->value();        // угол вокруг eta
    double p_deg      = ui->pSpinBox->value();            // угол вокруг z
    double maxMag     = ui->maxMagnitudeSpinBox->value(); // порог по яркости

    // Поле зрения (полуширины)
    double fovX_deg = ui->fovXSpinBox->value();
    double fovY_deg = ui->fovYSpinBox->value();

    // 2) Перевод в радианы
    double alpha0 = alpha0_deg * M_PI / 180.0;
    double dec0   = dec0_deg   * M_PI / 180.0;
    double beta1  = beta1_deg  * M_PI / 180.0;
    double beta2  = beta2_deg  * M_PI / 180.0;
    double p      = p_deg      * M_PI / 180.0;
    double fovX   = (fovX_deg/2.0) * M_PI / 180.0;
    double fovY   = (fovY_deg/2.0) * M_PI / 180.0;

    // 3) Проецируем все объекты (звёзды + Солнце ID=0)
    auto starProjections = catalog->projectStars(
        alpha0, dec0,
        /* p0 = */ 0.0,
        beta1, beta2, p,
        fovX, fovY,
        maxMag
        );

    // 4) Выбираем координаты Солнца
    double sunXi = 0.0, sunEta = 0.0;
    for (auto &pr : starProjections) {
        if (pr.starId == 0) {
            sunXi  = pr.x;
            sunEta = pr.y;
            break;
        }
    }

    // 5) Собираем обычные звёзды
    std::vector<double> xCoords, yCoords, mags;
    xCoords.reserve(starProjections.size());
    yCoords.reserve(starProjections.size());
    mags.   reserve(starProjections.size());
    for (auto &pr : starProjections) {
        if (pr.starId == 0) continue;
        xCoords.push_back(pr.x);
        yCoords.push_back(pr.y);
        mags.   push_back(pr.magnitude);
    }

    // 6) Очищаем старый виджет из MapWidget
    QLayout *mapLayout = ui->MapWidget->layout();
    if (mapLayout) {
        QLayoutItem *child;
        while ((child = mapLayout->takeAt(0)) != nullptr) {
            if (auto w = child->widget()) delete w;
            delete child;
        }
    }

    // 7) Создаём новый StarMapWidget
    StarMapWidget* mapWidget = new StarMapWidget(
        xCoords,
        yCoords,
        mags,
        ui->MapWidget
        );

    // 8) Передаём координаты Солнца
    mapWidget->setSunPosition(sunXi, sunEta);

    // 9) Добавляем его в лэйаут без пересоздания самого лэйаута
    mapLayout->addWidget(mapWidget);
}

void MainWindow::onAnglesChanged()
{
    double theta_deg = ui->beta1SpinBox->value();
    double psi_deg   = ui->beta2SpinBox->value();
    double phi_deg   = ui->pSpinBox->value();

    ui->GLWidget->setAngles(theta_deg, psi_deg, phi_deg);
}
