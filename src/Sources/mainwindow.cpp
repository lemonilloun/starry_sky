#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "StarCatalog.h"
#include "StarMapWidget.h"
#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDebug>
#include <cmath>
#include <cstdint>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , catalog(std::make_unique<StarCatalog>("/Users/lehacho/starry_sky/src/data/Catalogue_clean.csv"))
    , m_blurParams   { 13,   2.0, 1.0, 0.75 }
    , m_flareParams  { 0.6,  1.2, 128, 0.2, 0.3, 52.0 }
    , m_blurEnabled(true)
    , m_flareEnabled(true)
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

void MainWindow::on_settingsButton_clicked() {
    SettingsDialog dlg(this);
    dlg.setBlurParams (m_blurParams);
    dlg.setBlurEnabled(m_blurEnabled);
    dlg.setFlareParams(m_flareParams);
    dlg.setFlareEnabled(m_flareEnabled);
    if (dlg.exec() == QDialog::Accepted) {
        m_blurParams  = dlg.blurParams();
        m_flareParams = dlg.flareParams();
        m_blurEnabled  = dlg.blurEnabled();
        m_flareEnabled = dlg.flareEnabled();
        buildStarMap();
    }
}

void MainWindow::buildStarMap()
{
    // 1) Считываем значения углов из UI
    double alpha0_deg = ui->observerRaSpinBox->value();  // RA для визирования
    double dec0_deg   = ui->observerDecSpinBox->value(); // Dec для визирования
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
    double p0     = 0;
    double beta1  = beta1_deg  * M_PI / 180.0;
    double beta2  = beta2_deg  * M_PI / 180.0;
    double p      = p_deg      * M_PI / 180.0;
    double fovX   = fovX_deg   * M_PI / 180.0;
    double fovY   = fovY_deg   * M_PI / 180.0;

    // 3) Проецируем
    StarCatalog::Sun sunInfo;
    auto starProjections = catalog->projectStars(
        alpha0, dec0, p0,
        beta1,  beta2, p,
        fovX,   fovY,
        maxMag,
        sunInfo
        );

    // 4) Подготовим данные для StarMapWidget
    std::vector<double>   xCoords, yCoords, mags;
    std::vector<uint64_t> ids;                  // <-- вот он!
    xCoords.reserve(starProjections.size());
    yCoords.reserve(starProjections.size());
    mags.   reserve(starProjections.size());
    ids.    reserve(starProjections.size());

    for (auto &proj : starProjections) {
        xCoords.push_back(proj.x);
        yCoords.push_back(proj.y);
        mags.   push_back(proj.magnitude);
        ids.    push_back(proj.starId);         // <-- заполняем здесь
    }

    // 5) Очищаем старый виджет
    QLayout *mapLayout = ui->MapWidget->layout();
    if (mapLayout) {
        QLayoutItem *child;
        while ((child = mapLayout->takeAt(0)) != nullptr) {
            if (child->widget()) delete child->widget();
            delete child;
        }
    }

    // 6) Создаем новый и передаем ids
    auto *mapWidget = new StarMapWidget(
        xCoords,
        yCoords,
        mags,
        ids,
        sunInfo,            // ← pass the computed Sun object
        m_blurParams,
        m_blurEnabled,
        m_flareParams,
        m_flareEnabled,
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
