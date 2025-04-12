#include "StarMapWidget.h"
#include "GaussianBlur.h"
#include "LightPollution.h"
#include <QPainter>
#include <QImage>
#include <QFileDialog>
#include <QMessageBox>
#include <QWheelEvent>
#include <QVBoxLayout>
#include <limits>
#include <algorithm>

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

    // 1) Найдём min/max по X и по Y
    double minX = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();

    for (size_t i = 0; i < xCoords.size(); ++i) {
        double x = xCoords[i];
        double y = yCoords[i];
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
    }

    // Если вдруг звёзд нет или все координаты одинаковые (крайний случай)
    if (xCoords.empty() || minX == maxX || minY == maxY) {
        // Можно просто вернуть или нарисовать текст "No stars"
        painter.drawText(starMapImage.rect(), Qt::AlignCenter, "No stars to display");
        return;
    }

    // 2) Вычислим ширину/высоту bounding box
    double widthBBox  = maxX - minX;
    double heightBBox = maxY - minY;

    // 3) Берём размеры starMapImage (или вы можете взять widget'а):
    int imageW = starMapImage.width();
    int imageH = starMapImage.height();

    // 4) Вычислим масштаб по X и по Y (чтобы всё поместить)
    double scaleX = imageW / widthBBox;
    double scaleY = imageH / heightBBox;

    // 5) Если хотим «сохранять пропорции», берём общий scale = min(scaleX, scaleY)
    double scale = std::min(scaleX, scaleY);

    // 6) Считаем offset: мы хотим, чтобы (minX, minY) соответствовало (0,0) пикселей;
    //    либо можно сдвинуть так, чтобы звёзды оказались по центру.
    //    В самом простом случае:
    double offsetX = -minX;
    double offsetY = -minY;

    // 7) В цикле рисуем звёзды, переводя координаты:
    const double brightnessScale = 15.0;
    const int minBrightness = 60;
    const double maxStarRadius = 5.0;

    for (size_t i = 0; i < xCoords.size(); ++i) {
        double x = xCoords[i];
        double y = yCoords[i];
        double mag = magnitudes[i];

        // Преобразуем x,y к пиксельным координатам
        // Сначала сдвиг, потом умножаем на scale
        //double screenX = (x + offsetX) * scale;
        //double screenY = (y + offsetY) * scale;

        // Можно дополнительно «центрировать» по image:
        double screenX = ((x + offsetX)*scale) + (imageW - widthBBox*scale)/2;
        double screenY = ((y + offsetY)*scale) + (imageH - heightBBox*scale)/2;
        //
        // Но это опционально, если вы хотите звёзды в центре.

        // Вычислим яркость
        double brightnessFactor = brightnessScale / std::pow(2.512, mag);
        int brightness = static_cast<int>(std::clamp(brightnessFactor*255, 0.0, 255.0));
        brightness = std::max(brightness, minBrightness);

        QColor starColor(brightness, brightness, brightness);
        painter.setPen(starColor);
        painter.setBrush(starColor);

        double starRadius = std::clamp(1.5 * std::sqrt(brightnessFactor), 1.0, maxStarRadius);

        // Рисуем эллипс в точке (screenX, screenY)
        painter.drawEllipse(QPointF(screenX, screenY), starRadius, starRadius);
    }
}

void StarMapWidget::paintEvent(QPaintEvent* /* event */) {
    QPainter widgetPainter(this);
    widgetPainter.drawImage(rect(), blurredImage);

}
