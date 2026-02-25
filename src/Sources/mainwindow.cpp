#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "StarCatalog.h"
#include "StarMapWidget.h"
#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDebug>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , catalog(std::make_unique<StarCatalog>("/Users/lehacho/starry_sky/src/data/Catalogue_clean_extd.csv"))
    , m_blurParams   { 13,   0.1, 0.1, 0.0 }
    , m_flareParams  { 0.6,  1.2, 128, 0.2, 0.3, 52.0 }
    , m_blurEnabled(true)
    , m_flareEnabled(true)
    , m_planetSizeMode(PlanetRenderSizeMode::Real)
{
    ui->setupUi(this);

    QFont f = this->font();        // берём текущий системный шрифт
    f.setPointSize(18);            // ставим, например, 12 pt вместо стандартных ~8–9
    this->setFont(f);

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

    ui->DaySpinBox->setRange(1, 31);
    ui->maxMagnitudeSpinBox->setValue(4.5);
    ui->fovXSpinBox->setValue(20.0);
    ui->fovYSpinBox->setValue(20.0);
    ui->DaySpinBox->setValue(2);
    ui->MonthSpinBox->setValue(4);
    ui->YearSpinBox->setValue(1991);


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
    dlg.setPlanetSizeMode(m_planetSizeMode);
    if (dlg.exec() == QDialog::Accepted) {
        m_blurParams  = dlg.blurParams();
        m_flareParams = dlg.flareParams();
        m_blurEnabled  = dlg.blurEnabled();
        m_flareEnabled = dlg.flareEnabled();
        m_planetSizeMode = dlg.planetSizeMode();
        buildStarMap();
    }
}

void MainWindow::buildStarMap()
{
    resetZoomFromUiAndBuild();
}

ZoomViewParams MainWindow::readViewParamsFromUi() const
{
    ZoomViewParams view;
    view.alpha0RadJ2000 = ui->observerRaSpinBox->value() * M_PI / 180.0;
    view.dec0RadJ2000 = ui->observerDecSpinBox->value() * M_PI / 180.0;
    view.p0Rad = 0.0;
    view.beta1Rad = ui->beta1SpinBox->value() * M_PI / 180.0;
    view.beta2Rad = ui->beta2SpinBox->value() * M_PI / 180.0;
    view.pRad = ui->pSpinBox->value() * M_PI / 180.0;
    view.fovXRad = ui->fovXSpinBox->value() * M_PI / 180.0;
    view.fovYRad = ui->fovYSpinBox->value() * M_PI / 180.0;
    view.maxMagnitude = ui->maxMagnitudeSpinBox->value();
    view.obsDay = ui->DaySpinBox->value();
    view.obsMonth = ui->MonthSpinBox->value();
    view.obsYear = ui->YearSpinBox->value();
    return view;
}

void MainWindow::resetZoomFromUiAndBuild()
{
    const ZoomViewParams base = readViewParamsFromUi();
    m_zoomState.active = false;
    m_zoomState.factor = 2.0;
    m_zoomState.baseView = base;
    m_zoomState.currentView = base;
    buildStarMapWithParams(base);
}

void MainWindow::buildStarMapWithParams(const ZoomViewParams& view)
{
    StarCatalog::Sun sunInfo;
    auto starProjections = catalog->projectStars(
        view.alpha0RadJ2000,
        view.dec0RadJ2000,
        view.p0Rad,
        view.beta1Rad,
        view.beta2Rad,
        view.pRad,
        view.fovXRad,
        view.fovYRad,
        view.maxMagnitude,
        sunInfo,
        view.obsDay,
        view.obsMonth,
        view.obsYear
        );

    QLayout *mapLayout = ui->MapWidget->layout();
    if (mapLayout) {
        QLayoutItem *child;
        while ((child = mapLayout->takeAt(0)) != nullptr) {
            if (child->widget()) delete child->widget();
            delete child;
        }
    }

    ObservationInfo obsInfo;
    obsInfo.observerRaRad = view.alpha0RadJ2000;
    obsInfo.observerDecRad = view.dec0RadJ2000;
    obsInfo.fovXRad = view.fovXRad;
    obsInfo.fovYRad = view.fovYRad;
    obsInfo.obsDay = view.obsDay;
    obsInfo.obsMonth = view.obsMonth;
    obsInfo.obsYear = view.obsYear;

    auto *mapWidget = new StarMapWidget(
        std::move(starProjections),
        sunInfo,
        m_blurParams,
        m_blurEnabled,
        m_flareParams,
        m_flareEnabled,
        m_planetSizeMode,
        obsInfo,
        m_zoomState.active,
        m_zoomState.active ? m_zoomState.factor : 1.0,
        ui->MapWidget
        );

    connect(mapWidget, &StarMapWidget::zoomToggleRequested,
            this, &MainWindow::onZoomToggleRequested);
    connect(mapWidget, &StarMapWidget::zoomStepRequested,
            this, &MainWindow::onZoomStepRequested);
    connect(mapWidget, &StarMapWidget::zoomCenterRequested,
            this, &MainWindow::onZoomCenterRequested);
    connect(mapWidget, &StarMapWidget::zoomExitRequested,
            this, &MainWindow::onZoomExitRequested);

    mapLayout->addWidget(mapWidget);
    mapWidget->setFocus();
    starMapWidget = mapWidget;
}

void MainWindow::applyZoomAndRebuild()
{
    if (!m_zoomState.active)
        return;

    const double factor = std::clamp(m_zoomState.factor, 1.0, 20.0);
    m_zoomState.factor = factor;

    ZoomViewParams zoomed = m_zoomState.currentView;
    const ZoomViewParams& base = m_zoomState.baseView;
    zoomed.fovXRad = std::atan(std::tan(base.fovXRad) / factor);
    zoomed.fovYRad = std::atan(std::tan(base.fovYRad) / factor);

    const double boost = 2.5 * std::log10(std::max(1.0, factor));
    zoomed.maxMagnitude = std::min(base.maxMagnitude + 3.5, base.maxMagnitude + boost);

    buildStarMapWithParams(zoomed);
}

void MainWindow::onZoomToggleRequested()
{
    if (!m_zoomState.active) {
        m_zoomState.active = true;
        m_zoomState.factor = 2.0;
        m_zoomState.currentView = m_zoomState.baseView;
        applyZoomAndRebuild();
        return;
    }

    m_zoomState.active = false;
    m_zoomState.factor = 2.0;
    m_zoomState.currentView = m_zoomState.baseView;
    buildStarMapWithParams(m_zoomState.baseView);
}

void MainWindow::onZoomStepRequested(int direction)
{
    if (!m_zoomState.active)
        return;

    if (direction > 0)
        m_zoomState.factor = std::min(20.0, m_zoomState.factor * 1.25);
    else if (direction < 0)
        m_zoomState.factor = std::max(1.0, m_zoomState.factor / 1.25);
    applyZoomAndRebuild();
}

void MainWindow::onZoomCenterRequested(double xi, double eta)
{
    if (!m_zoomState.active)
        return;

    double newRa = 0.0;
    double newDec = 0.0;
    if (!zoominspector::computeNewCenterFromClick(m_zoomState.currentView, xi, eta, newRa, newDec)) {
        qWarning() << "[ZoomInspector] unable to compute new center from click";
        return;
    }

    m_zoomState.currentView.alpha0RadJ2000 = newRa;
    m_zoomState.currentView.dec0RadJ2000 = newDec;
    applyZoomAndRebuild();
}

void MainWindow::onZoomExitRequested()
{
    if (!m_zoomState.active)
        return;

    m_zoomState.active = false;
    m_zoomState.factor = 2.0;
    m_zoomState.currentView = m_zoomState.baseView;
    buildStarMapWithParams(m_zoomState.baseView);
}

void MainWindow::onAnglesChanged()
{
    double theta_deg = ui->beta1SpinBox->value();
    double psi_deg   = ui->beta2SpinBox->value();
    double phi_deg   = ui->pSpinBox->value();

    ui->GLWidget->setAngles(-theta_deg, psi_deg, phi_deg);
}
