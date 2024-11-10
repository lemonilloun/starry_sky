#include "LightPollution.h"
#include <QImage>
#include <cmath>
#include <algorithm>

QImage LightPollution::applyLightPollution(const QImage& image, double intensity, double gradientFactor) {
    int width = image.width();
    int height = image.height();
    QImage resultImage = image;

    for (int y = 0; y < height; ++y) {
        // Вычисляем фактор градиента для текущей строки y (чем ниже, тем больше засветка)
        double factor = intensity * std::exp(-gradientFactor * (1.0 - static_cast<double>(y) / height));

        for (int x = 0; x < width; ++x) {
            int grayValue = qGray(resultImage.pixel(x, y));  // Получаем текущую яркость (0-255)

            // Увеличиваем яркость на основе градиентного фактора
            int newGrayValue = std::clamp(static_cast<int>(grayValue + factor * 255), 0, 255);

            // Устанавливаем обновленный уровень яркости обратно в изображение
            resultImage.setPixel(x, y, qRgb(newGrayValue, newGrayValue, newGrayValue));
        }
    }

    return resultImage;
}
