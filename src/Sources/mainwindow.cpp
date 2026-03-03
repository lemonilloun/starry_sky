#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "StarCatalog.h"
#include "StarMapWidget.h"
#include "SettingsDialog.h"
#include "SimulationDialog.h"
#include "SimulationPlayerWidget.h"
#include "CelestialBodyTypes.h"
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDebug>
#include <QProgressDialog>
#include <QDate>
#include <QCoreApplication>
#include <QLabel>
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
    connect(ui->SimulateButton, &QPushButton::clicked,
            this, &MainWindow::onSimulateClicked);

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
    if (m_simulationRunning)
        return;
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

void MainWindow::clearMapWidgetLayout()
{
    QLayout* mapLayout = ui->MapWidget->layout();
    if (!mapLayout)
        return;

    QLayoutItem* child = nullptr;
    while ((child = mapLayout->takeAt(0)) != nullptr) {
        if (child->widget())
            delete child->widget();
        delete child;
    }
}

void MainWindow::setSimulationUiLocked(bool locked)
{
    const bool enabled = !locked;
    ui->observerRaSpinBox->setEnabled(enabled);
    ui->observerDecSpinBox->setEnabled(enabled);
    ui->beta1SpinBox->setEnabled(enabled);
    ui->beta2SpinBox->setEnabled(enabled);
    ui->pSpinBox->setEnabled(enabled);
    ui->fovXSpinBox->setEnabled(enabled);
    ui->fovYSpinBox->setEnabled(enabled);
    ui->maxMagnitudeSpinBox->setEnabled(enabled);
    ui->DaySpinBox->setEnabled(enabled);
    ui->MonthSpinBox->setEnabled(enabled);
    ui->YearSpinBox->setEnabled(enabled);
    ui->buildMapButton->setEnabled(enabled);
    ui->settingsButton->setEnabled(enabled);
    ui->SimulateButton->setEnabled(enabled);
}

void MainWindow::buildStarMapWithParams(const ZoomViewParams& view)
{
    if (m_simulationRunning)
        return;

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

    clearMapWidgetLayout();
    QLayout* mapLayout = ui->MapWidget->layout();
    if (!mapLayout)
        return;

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
    m_simulationPlayer = nullptr;
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

void MainWindow::onSimulateClicked()
{
    if (m_simulationRunning)
        return;

    const QDate defaultDate(ui->YearSpinBox->value(), ui->MonthSpinBox->value(), ui->DaySpinBox->value());
    SimulationDialog dialog(defaultDate.isValid() ? defaultDate : QDate::currentDate(), this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const SimulationRequest req = dialog.request();
    if (!req.startDate.isValid() || !req.endDate.isValid() || req.startDate > req.endDate) {
        QMessageBox::warning(this, QStringLiteral("Invalid range"),
                             QStringLiteral("Start date must be earlier or equal to end date."));
        return;
    }

    QVector<QDate> dates;
    for (QDate d = req.startDate; d <= req.endDate; d = d.addDays(std::max(1, req.stepDays)))
        dates.push_back(d);

    if (dates.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("No frames"),
                             QStringLiteral("No frames were generated for selected date range."));
        return;
    }

    if (dates.size() > 1500) {
        const auto answer = QMessageBox::question(
            this,
            QStringLiteral("Large simulation"),
            QStringLiteral("This simulation will generate %1 frames. Continue?")
                .arg(dates.size())
        );
        if (answer != QMessageBox::Yes)
            return;
    }

    const ZoomViewParams baseView = readViewParamsFromUi();
    m_simulationRunning = true;
    m_lastSimulationRequest = req;
    m_simFrames.clear();
    setSimulationUiLocked(true);

    QProgressDialog progress(QStringLiteral("Precomputing simulation frames..."),
                             QStringLiteral("Cancel"),
                             0,
                             dates.size(),
                             this);
    progress.setWindowModality(Qt::ApplicationModal);
    progress.setMinimumDuration(0);
    progress.setValue(0);

    int skippedFrames = 0;
    bool canceled = false;

    for (int i = 0; i < dates.size(); ++i) {
        if (progress.wasCanceled()) {
            canceled = true;
            break;
        }

        const QDate d = dates[i];
        progress.setLabelText(QStringLiteral("Precomputing %1 / %2 (%3)")
                              .arg(i + 1)
                              .arg(dates.size())
                              .arg(d.toString(QStringLiteral("yyyy-MM-dd"))));
        progress.setValue(i);
        QCoreApplication::processEvents();

        double raJ2000 = 0.0;
        double decJ2000 = 0.0;
        if (!catalog->computeBodyCenterJ2000(
                req.body,
                d.day(),
                d.month(),
                d.year(),
                raJ2000,
                decJ2000
                )) {
            ++skippedFrames;
            continue;
        }

        ZoomViewParams frameView = baseView;
        frameView.alpha0RadJ2000 = raJ2000;
        frameView.dec0RadJ2000 = decJ2000;
        frameView.beta1Rad = 0.0;
        frameView.beta2Rad = 0.0;
        frameView.pRad = 0.0;
        frameView.obsDay = d.day();
        frameView.obsMonth = d.month();
        frameView.obsYear = d.year();

        StarCatalog::Sun sunInfo;
        auto frameProjections = catalog->projectStars(
            frameView.alpha0RadJ2000,
            frameView.dec0RadJ2000,
            frameView.p0Rad,
            frameView.beta1Rad,
            frameView.beta2Rad,
            frameView.pRad,
            frameView.fovXRad,
            frameView.fovYRad,
            frameView.maxMagnitude,
            sunInfo,
            frameView.obsDay,
            frameView.obsMonth,
            frameView.obsYear
            );

        ObservationInfo obsInfo;
        obsInfo.observerRaRad = frameView.alpha0RadJ2000;
        obsInfo.observerDecRad = frameView.dec0RadJ2000;
        obsInfo.fovXRad = frameView.fovXRad;
        obsInfo.fovYRad = frameView.fovYRad;
        obsInfo.obsDay = frameView.obsDay;
        obsInfo.obsMonth = frameView.obsMonth;
        obsInfo.obsYear = frameView.obsYear;

        StarMapWidget frameWidget(
            std::move(frameProjections),
            sunInfo,
            m_blurParams,
            m_blurEnabled,
            m_flareParams,
            m_flareEnabled,
            m_planetSizeMode,
            obsInfo,
            false,
            1.0,
            nullptr
            );

        const QImage frame = frameWidget.renderedImage();
        if (frame.isNull()) {
            ++skippedFrames;
            continue;
        }
        m_simFrames.push_back(frame);
    }

    progress.setValue(dates.size());

    if (canceled || m_simFrames.isEmpty()) {
        m_simulationRunning = false;
        setSimulationUiLocked(false);
        if (!canceled) {
            QMessageBox::warning(this,
                                 QStringLiteral("Simulation"),
                                 QStringLiteral("No frames were generated."));
        }
        return;
    }

    if (skippedFrames > 0) {
        qWarning() << "[Simulation] skipped frames:" << skippedFrames;
    }

    m_zoomState.active = false;
    m_zoomState.factor = 2.0;

    clearMapWidgetLayout();
    QLayout* mapLayout = ui->MapWidget->layout();
    if (!mapLayout) {
        m_simulationRunning = false;
        setSimulationUiLocked(false);
        return;
    }

    auto* player = new SimulationPlayerWidget(ui->MapWidget);
    player->setFrames(m_simFrames, req.playbackMsPerFrame);
    connect(player, &SimulationPlayerWidget::finished,
            this, &MainWindow::onSimulationPlaybackFinished);

    mapLayout->addWidget(player);
    m_simulationPlayer = player;
    starMapWidget = nullptr;
    player->start();
}

void MainWindow::onSimulationPlaybackFinished()
{
    if (!m_simulationRunning)
        return;

    if (m_simulationPlayer)
        m_simulationPlayer->stop();

    m_simulationRunning = false;
    m_simFrames.clear();
    m_simulationPlayer = nullptr;
    setSimulationUiLocked(false);
    buildStarMap();
}

void MainWindow::onAnglesChanged()
{
    double theta_deg = ui->beta1SpinBox->value();
    double psi_deg   = ui->beta2SpinBox->value();
    double phi_deg   = ui->pSpinBox->value();

    ui->GLWidget->setAngles(-theta_deg, psi_deg, phi_deg);
}
