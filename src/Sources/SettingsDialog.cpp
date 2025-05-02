#include "SettingsDialog.h"
#include <QFormLayout>
#include <QSpinBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QCheckBox>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    auto *form = new QFormLayout(this);

    //
    // === Секция 1: Параметры Гауссова фильтра ===
    //
    // Добавляем заголовок-сепаратор
    form->addRow(new QLabel("<b>Параметры Гауссова фильтра</b>"));

    // чекбокс включения/выключения размытия
    m_blurEnabled = new QCheckBox("Активировать размытие");
    m_blurEnabled->setChecked(true);
    form->addRow(m_blurEnabled);

    // kernelSize — размер ядра свёртки (радиус размытия в px, обычно нечетное число)
    m_kernelSize = new QSpinBox;
    m_kernelSize->setRange(1, 101);
    m_kernelSize->setValue(13);
    form->addRow("Размер ядра свёртки", m_kernelSize);

    // sigmaX — стандартное отклонение по оси X (чем больше, тем сильнее размытие)
    m_sigmaX = new QDoubleSpinBox;
    m_sigmaX->setRange(0.1, 10.0);
    m_sigmaX->setValue(2.0);
    form->addRow("σ_x (направлени смаза по X)", m_sigmaX);

    // sigmaY — стандартное отклонение по оси Y
    m_sigmaY = new QDoubleSpinBox;
    m_sigmaY->setRange(0.1, 10.0);
    m_sigmaY->setValue(1.0);
    form->addRow("σ_Y (направлени смаза по Y)", m_sigmaY);

    // rho — коэффициент корреляции осей (0 = независимые фильтры, 1 = одинаковые)
    m_rho = new QDoubleSpinBox;
    m_rho->setRange(0.0, 1.0);
    m_rho->setValue(0.75);
    form->addRow("ρ (степень смаза)", m_rho);

    //
    // === Секция 2: Параметры засветки (flare) ===
    //
    form->addRow(new QLabel("<b>Параметры засветки</b>"));

    m_flareEnabled = new QCheckBox("Активировать засветку");
    m_flareEnabled->setChecked(true);
    form->addRow(m_flareEnabled);

    // baseIntensity — начальная (центральная) яркость ореола [0..1]
    m_baseIntensity = new QDoubleSpinBox;
    m_baseIntensity->setRange(0.0, 1.0);
    m_baseIntensity->setValue(0.4);
    form->addRow("Яркость ореола", m_baseIntensity);

    // baseRadiusFactor — радиус ореола как доля от max(ширина, высота)
    m_baseRadiusFactor = new QDoubleSpinBox;
    m_baseRadiusFactor->setRange(0.1, 2.0);
    m_baseRadiusFactor->setValue(1.0);
    form->addRow("Радиус ореола", m_baseRadiusFactor);

    // numRays — количество «лучей» блика
    m_numRays = new QSpinBox;
    m_numRays->setRange(0, 512);
    m_numRays->setValue(128);
    form->addRow("Число лучей", m_numRays);

    // rayIntensity — яркость начала каждого луча [0..1]
    m_rayIntensity = new QDoubleSpinBox;
    m_rayIntensity->setRange(0.0, 1.0);
    m_rayIntensity->setValue(0.2);
    form->addRow("Яркость луча", m_rayIntensity);

    // maxRayLengthFactor — максимальная длина лучей как доля от max(ширина, высота)
    m_maxRayLengthFactor = new QDoubleSpinBox;
    m_maxRayLengthFactor->setRange(0.0, 1.0);
    m_maxRayLengthFactor->setValue(0.3);
    form->addRow("Длина лучей", m_maxRayLengthFactor);

    // coreRadius — радиус твёрдого светящегося ядра (px)
    m_coreRadius = new QDoubleSpinBox;
    m_coreRadius->setRange(1.0, 200.0);
    m_coreRadius->setValue(52.0);
    form->addRow("Радиус ядра Солнца", m_coreRadius);

    // Кнопки ОК/Отмена
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addWidget(m_buttons);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
}

BlurParams SettingsDialog::blurParams() const {
    return {
        m_kernelSize->value(),
        m_sigmaX->value(),
        m_sigmaY->value(),
        m_rho->value()
    };
}

FlareParams SettingsDialog::flareParams() const {
    return {
        m_baseIntensity->value(),
        m_baseRadiusFactor->value(),
        m_numRays->value(),
        m_rayIntensity->value(),
        m_maxRayLengthFactor->value(),
        m_coreRadius->value()
    };
}

void SettingsDialog::setBlurParams(const BlurParams& p) {
    m_kernelSize->setValue(p.kernelSize);
    m_sigmaX    ->setValue(p.sigmaX);
    m_sigmaY    ->setValue(p.sigmaY);
    m_rho       ->setValue(p.rho);
}

void SettingsDialog::setFlareParams(const FlareParams& p) {
    m_baseIntensity    ->setValue(p.baseIntensity);
    m_baseRadiusFactor ->setValue(p.baseRadiusFactor);
    m_numRays          ->setValue(p.numRays);
    m_rayIntensity     ->setValue(p.rayIntensity);
    m_maxRayLengthFactor->setValue(p.maxRayLengthFactor);
    m_coreRadius       ->setValue(p.coreRadius);
}

// методы доступа к чекбоксам:
bool SettingsDialog::blurEnabled()  const { return m_blurEnabled->isChecked(); }
void SettingsDialog::setBlurEnabled(bool e)   { m_blurEnabled->setChecked(e); }
bool SettingsDialog::flareEnabled() const { return m_flareEnabled->isChecked(); }
void SettingsDialog::setFlareEnabled(bool e) { m_flareEnabled->setChecked(e); }
