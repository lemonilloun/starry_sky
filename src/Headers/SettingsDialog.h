#pragma once
#include <QDialog>

class QSpinBox;
class QDoubleSpinBox;
class QDialogButtonBox;
class QCheckBox;
class QComboBox;

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

enum class PlanetRenderSizeMode {
    Real = 0,
    Enhanced = 1
};

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    SettingsDialog(QWidget* parent = nullptr);

    BlurParams  blurParams() const;
    FlareParams flareParams() const;

    bool blurEnabled()  const;
    bool flareEnabled() const;
    PlanetRenderSizeMode planetSizeMode() const;

    void setBlurEnabled(bool  e);
    void setFlareEnabled(bool e);
    void setPlanetSizeMode(PlanetRenderSizeMode mode);

    void setBlurParams (const BlurParams& p);
    void setFlareParams(const FlareParams& p);


private:
    // Виджеты для размытия
    QCheckBox*       m_blurEnabled;
    QSpinBox*       m_kernelSize;
    QDoubleSpinBox* m_sigmaX;
    QDoubleSpinBox* m_sigmaY;
    QDoubleSpinBox* m_rho;

    // Виджеты для flare
    QCheckBox*       m_flareEnabled;
    QDoubleSpinBox* m_baseIntensity;
    QDoubleSpinBox* m_baseRadiusFactor;
    QSpinBox*       m_numRays;
    QDoubleSpinBox* m_rayIntensity;
    QDoubleSpinBox* m_maxRayLengthFactor;
    QDoubleSpinBox* m_coreRadius;

    QComboBox*      m_planetSizeMode;

    QDialogButtonBox* m_buttons;
};
