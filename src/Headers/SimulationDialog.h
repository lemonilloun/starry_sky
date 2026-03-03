#pragma once

#include <QDialog>
#include "SimulationTypes.h"

class QComboBox;
class QDateEdit;
class QLabel;
class QSpinBox;

class SimulationDialog : public QDialog {
    Q_OBJECT

public:
    explicit SimulationDialog(const QDate& defaultDate, QWidget* parent = nullptr);

    SimulationRequest request() const;

private slots:
    void updateFrameEstimate();

private:
    QComboBox* m_bodyCombo = nullptr;
    QDateEdit* m_startDateEdit = nullptr;
    QDateEdit* m_endDateEdit = nullptr;
    QSpinBox* m_stepDaysSpin = nullptr;
    QSpinBox* m_playbackDelaySpin = nullptr;
    QLabel* m_estimateLabel = nullptr;
};
