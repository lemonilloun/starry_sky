#include "StarMapWidget.h"
#include "GaussianBlur.h"
#include "LightPollution.h"
#include <QPainter>
#include <QImage>
#include <QFileDialog>
#include <QMessageBox>
#include <QWheelEvent>
#include <QVBoxLayout>

StarMapWidget::StarMapWidget(const std::vector<double>& xCoords,
                             const std::vector<double>& yCoords,
                             const std::vector<double>& magnitudes,
                             const std::vector<double>& colorIndices,
                             QWidget* parent)
    : QWidget(parent)
    , xCoords(xCoords)
    , yCoords(yCoords)
    , magnitudes(magnitudes)
    , colorIndices(colorIndices)
{
    // Создаем изображение для рисования звездного неба (черно-белое изображение)
    starMapImage = QImage(1081, 761, QImage::Format_Grayscale8);
    starMapImage.fill(Qt::black);  // Черный фон

    // Рисуем изображение звездного неба
    renderStars();

    // Применяем фильтр Гаусса к изображению звездного неба
    blurredImage = GaussianBlur::applyGaussianBlur(starMapImage, 13, 2.0, 1.0, 0.60);  // гиперпараметры: размер ядра, sigmaX, sigmaY, rho
    //blurredImage = LightPollution::applyLightPollution(blurredImage, 0.0, 0.8);       // гиперпараметры: intensity, gradientFactor

}

void StarMapWidget::renderStars() {
    QPainter painter(&starMapImage);
    painter.setRenderHint(QPainter::Antialiasing);

    const double brightnessScale = 5.0;  // Коэффициент усиления яркости
    const int minBrightness = 30;        // Минимальная яркость, чтобы звезды не были слишком тусклыми
    const double maxStarRadius = 3.0;    // Максимальный радиус звезды

    for (size_t i = 0; i < xCoords.size(); ++i) {
        double screenX = starMapImage.width() / 2 + xCoords[i] * starMapImage.width() / 2;
        double screenY = starMapImage.height() / 2 - yCoords[i] * starMapImage.height() / 2;

        double brightnessFactor = brightnessScale / std::pow(2.512, magnitudes[i]);
        int brightness = static_cast<int>(std::clamp(brightnessFactor * 255, 0.0, 255.0));

        brightness = std::max(brightness, minBrightness);

        QColor starColor(brightness, brightness, brightness);
        painter.setPen(starColor);
        painter.setBrush(starColor);

        double starRadius = std::clamp(1.5 * std::sqrt(brightnessFactor), 1.0, maxStarRadius);
        painter.drawEllipse(QPointF(screenX, screenY), starRadius, starRadius);
    }
}

void StarMapWidget::paintEvent(QPaintEvent* /* event */) {
    QPainter widgetPainter(this);
    widgetPainter.drawImage(rect(), blurredImage);

}
