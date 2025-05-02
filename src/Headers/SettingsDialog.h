#pragma once
#include <QDialog>

class QSpinBox;
class QDoubleSpinBox;
class QDialogButtonBox;

struct FlareParams {
    double baseIntensity;
    double baseRadiusFactor;
    int    numRays;
    double rayIntensity;
    double maxRayLengthFactor;
    double coreRadius;
};

struct BlurParams {
    int    kernelSize;
    double sigmaX;
    double sigmaY;
    double rho;
};

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    SettingsDialog(QWidget* parent = nullptr);

    BlurParams  blurParams() const;
    FlareParams flareParams() const;

    void setBlurParams (const BlurParams& p);
    void setFlareParams(const FlareParams& p);

private:
    // Виджеты для размытия
    QSpinBox*       m_kernelSize;
    QDoubleSpinBox* m_sigmaX;
    QDoubleSpinBox* m_sigmaY;
    QDoubleSpinBox* m_rho;

    // Виджеты для flare
    QDoubleSpinBox* m_baseIntensity;
    QDoubleSpinBox* m_baseRadiusFactor;
    QSpinBox*       m_numRays;
    QDoubleSpinBox* m_rayIntensity;
    QDoubleSpinBox* m_maxRayLengthFactor;
    QDoubleSpinBox* m_coreRadius;

    QDialogButtonBox* m_buttons;
};
