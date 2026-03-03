#include "SimulationDialog.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSpinBox>
#include <QVBoxLayout>
#include <algorithm>

#include "CelestialBodyTypes.h"

namespace {
struct BodyEntry {
    astro::BodyId id;
    const char* label;
};

constexpr BodyEntry BODY_ENTRIES[] = {
    {astro::BodyId::Sun, "Sun"},
    {astro::BodyId::Moon, "Moon"},
    {astro::BodyId::Mercury, "Mercury"},
    {astro::BodyId::Venus, "Venus"},
    {astro::BodyId::Mars, "Mars"},
    {astro::BodyId::Jupiter, "Jupiter"},
    {astro::BodyId::Saturn, "Saturn"},
    {astro::BodyId::Uranus, "Uranus"},
    {astro::BodyId::Neptune, "Neptune"}
};
}

SimulationDialog::SimulationDialog(const QDate& defaultDate, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Simulate"));
    setModal(true);
    resize(420, 240);

    m_bodyCombo = new QComboBox(this);
    for (const auto& entry : BODY_ENTRIES) {
        m_bodyCombo->addItem(QString::fromLatin1(entry.label),
                             QVariant::fromValue<qulonglong>(astro::bodyIdValue(entry.id)));
    }
    m_bodyCombo->setCurrentIndex(1);

    m_startDateEdit = new QDateEdit(defaultDate, this);
    m_startDateEdit->setCalendarPopup(true);
    m_startDateEdit->setDisplayFormat("yyyy-MM-dd");

    m_endDateEdit = new QDateEdit(defaultDate.addDays(30), this);
    m_endDateEdit->setCalendarPopup(true);
    m_endDateEdit->setDisplayFormat("yyyy-MM-dd");

    m_stepDaysSpin = new QSpinBox(this);
    m_stepDaysSpin->setRange(1, 365);
    m_stepDaysSpin->setValue(1);

    m_playbackDelaySpin = new QSpinBox(this);
    m_playbackDelaySpin->setRange(10, 1000);
    m_playbackDelaySpin->setValue(40);
    m_playbackDelaySpin->setSuffix(" ms");

    m_estimateLabel = new QLabel(this);
    m_estimateLabel->setStyleSheet("color: #d6d6d6;");

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Body"), m_bodyCombo);
    form->addRow(QStringLiteral("Start date"), m_startDateEdit);
    form->addRow(QStringLiteral("End date"), m_endDateEdit);
    form->addRow(QStringLiteral("Step (days)"), m_stepDaysSpin);
    form->addRow(QStringLiteral("Playback delay"), m_playbackDelaySpin);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    auto* root = new QVBoxLayout(this);
    root->addLayout(form);
    root->addWidget(m_estimateLabel);
    root->addWidget(buttons);

    connect(m_startDateEdit, &QDateEdit::dateChanged, this, &SimulationDialog::updateFrameEstimate);
    connect(m_endDateEdit, &QDateEdit::dateChanged, this, &SimulationDialog::updateFrameEstimate);
    connect(m_stepDaysSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SimulationDialog::updateFrameEstimate);

    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        if (m_startDateEdit->date() > m_endDateEdit->date()) {
            QMessageBox::warning(this,
                                 QStringLiteral("Invalid range"),
                                 QStringLiteral("Start date must be earlier or equal to end date."));
            return;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &SimulationDialog::reject);

    updateFrameEstimate();
}

SimulationRequest SimulationDialog::request() const
{
    SimulationRequest req;
    req.body = static_cast<astro::BodyId>(
        m_bodyCombo->currentData().toULongLong()
    );
    req.startDate = m_startDateEdit->date();
    req.endDate = m_endDateEdit->date();
    req.stepDays = m_stepDaysSpin->value();
    req.playbackMsPerFrame = m_playbackDelaySpin->value();
    return req;
}

void SimulationDialog::updateFrameEstimate()
{
    const QDate start = m_startDateEdit->date();
    const QDate end = m_endDateEdit->date();
    const int step = std::max(1, m_stepDaysSpin->value());

    if (!start.isValid() || !end.isValid()) {
        m_estimateLabel->setText(QStringLiteral("Estimated frames: n/a"));
        return;
    }

    if (start > end) {
        m_estimateLabel->setText(QStringLiteral("Estimated frames: invalid date range"));
        return;
    }

    const int daysSpan = start.daysTo(end);
    const int frameCount = daysSpan / step + 1;
    m_estimateLabel->setText(QStringLiteral("Estimated frames: %1").arg(frameCount));
}
