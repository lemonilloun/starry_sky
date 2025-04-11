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
    QPixmap pix("/Users/lehacho/starry_sky/src/data/placeholder.png");
    placeholder->setPixmap(pix);

    ui->MapWidget->layout()->addWidget(placeholder);

    //ui->GLWidget->setAngles(0, 0, 0);

    ui->observerRaSpinBox->setValue(1.02703168676271);   // RA
    ui->observerDecSpinBox->setValue(-0.00360223021825306); // Dec
    ui->p0SpinBox->setValue(0.0);     // Начальный поворот вокруг оси визирования
    ui->beta1SpinBox->setValue(0.0);
    ui->beta2SpinBox->setValue(0.0);
    ui->pSpinBox->setValue(0.0);

    ui->maxMagnitudeSpinBox->setValue(3.8);
    ui->fovXSpinBox->setValue(58.0);
    ui->fovYSpinBox->setValue(47.0);


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
    double alpha0_deg = ui->observerRaSpinBox->value();  // RA для визирования
    double dec0_deg   = ui->observerDecSpinBox->value(); // Dec для визирования
    double p0_deg     = ui->p0SpinBox->value();          // начальный поворот матрицы
    double beta1_deg  = ui->beta1SpinBox->value();       // угол вокруг оси xi
    double beta2_deg  = ui->beta2SpinBox->value();       // угол вокруг оси eta
    double p_deg      = ui->pSpinBox->value();           // конечный поворот вокруг оси z (визирования)
    double maxMag     = ui->maxMagnitudeSpinBox->value();

    // Поле зрения
    double fovX_deg   = ui->fovXSpinBox->value(); // полуширина по X
    double fovY_deg   = ui->fovYSpinBox->value(); // полуширина по Y

    // 2) Перевод в радианы
    double alpha0 = alpha0_deg * M_PI / 180.0;
    double dec0   = dec0_deg   * M_PI / 180.0;
    double p0     = p0_deg     * M_PI / 180.0;
    double beta1  = beta1_deg  * M_PI / 180.0;
    double beta2  = beta2_deg  * M_PI / 180.0;
    double p      = p_deg      * M_PI / 180.0;
    double fovX   = fovX_deg   * M_PI / 180.0;
    double fovY   = fovY_deg   * M_PI / 180.0;

    // 3) Вызываем новый метод projectStars(...) из StarCatalog
    //    Он вернёт список StarProjection {x,y,magnitude}
    auto starProjections = catalog->projectStars(
        alpha0, dec0, p0,
        beta1, beta2, p,
        fovX, fovY,
        maxMag
        );

    // 4) Подготовим данные, чтобы StarMapWidget мог их отрисовать
    //    (xCoords, yCoords, magnitudes)
    std::vector<double> xCoords;
    std::vector<double> yCoords;
    std::vector<double> mags;
    xCoords.reserve(starProjections.size());
    yCoords.reserve(starProjections.size());
    mags.reserve(starProjections.size());

    for (auto &proj : starProjections) {
        xCoords.push_back(proj.x);
        yCoords.push_back(proj.y);
        mags.push_back(proj.magnitude);
    }

    // 5) Удаляем старое содержимое MapWidget (убираем заглушку или прежний StarMapWidget)
    QLayout *mapLayout = ui->MapWidget->layout();
    if (mapLayout) {
        QLayoutItem *child;
        while ((child = mapLayout->takeAt(0)) != nullptr) {
            if (child->widget()) {
                delete child->widget();
            }
            delete child;
        }
    }

    // 6) Создаём StarMapWidget на основе xCoords,yCoords
    StarMapWidget* mapWidget = new StarMapWidget(
        xCoords,
        yCoords,
        mags,
        /* colorIndices = */ std::vector<double>(),
        ui->MapWidget
        );

    // 7) Добавляем в лэйаут
    mapLayout->addWidget(mapWidget);

}

void MainWindow::onAnglesChanged()
{
    double theta_deg = ui->beta1SpinBox->value();
    double psi_deg   = ui->beta2SpinBox->value();
    double phi_deg   = ui->pSpinBox->value();

    ui->GLWidget->setAngles(theta_deg, psi_deg, phi_deg);
}
