#include "LightPollution.h"
#include <QRandomGenerator>
#include <QImage>
#include <cmath>
#include <algorithm>


QImage LightPollution::applyLightPollution(const QImage& image, double intensity, double gradientFactor) {
    int width = image.width();
    int height = image.height();
    QImage resultImage = image;

    // Случайная интенсивность засветки в диапазоне от 0.1 до 0.3
    if (intensity <= 0.0) {
        intensity = 0.1 + QRandomGenerator::global()->bounded(0.2);
    }

    // Генерируем случайную центральную точку для засветки
    int centerX = QRandomGenerator::global()->bounded(width);
    int centerY = QRandomGenerator::global()->bounded(height);

    // Максимальное расстояние для нормализации - возможно стоит изменить, так как это норм, если засвет выходит да границы изображения
    double maxDistance = std::sqrt(centerX * centerX + centerY * centerY);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int grayValue = qGray(resultImage.pixel(x, y));  // Получаем текущую яркость (0-255)

            // Вычисляем расстояние от текущего пикселя до центра засветки
            double distance = std::sqrt(std::pow(x - centerX, 2) + std::pow(y - centerY, 2));

            // Гауссово затухание
            double gaussianFactor = intensity * std::exp(-std::pow(distance / (maxDistance * gradientFactor), 2));

            // Получаем случайное значение в диапазоне [-0.02, 0.02]
            double noise = (QRandomGenerator::global()->bounded(4001) - 2000) / 100000.0 * 255;

            // Итоговая яркость с добавлением шума
            int newGrayValue = std::clamp(static_cast<int>(grayValue + gaussianFactor * 255 + noise), 0, 255);

            // Устанавливаем обновленный уровень яркости обратно в изображение
            resultImage.setPixel(x, y, qRgb(newGrayValue, newGrayValue, newGrayValue));
        }
    }

    return resultImage;
}
