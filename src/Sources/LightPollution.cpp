// LightPollution.cpp  (в каталоге src/Utils)
#include "LightPollution.h"
#include <cmath>
#include <algorithm>
#include <QColor>
#include <QtGlobal>

QImage LightPollution::applySunFlare(
    const QImage &image,
    double centerX,
    double centerY,
    double intensity,
    double gradientFac
    ) {
    int W = image.width(), H = image.height();
    QImage result = image;

    // Найдём максимальную дистанцию до угла кадра
    double dx = std::max(centerX, W - centerX);
    double dy = std::max(centerY, H - centerY);
    double maxDist = std::hypot(dx, dy);

    // Ограничим intensity
    intensity = std::clamp(intensity, 0.0, 1.0);

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int gray = qGray(image.pixel(x, y));
            double dist = std::hypot(x - centerX, y - centerY);
            double f = intensity * std::exp(-std::pow(dist / (maxDist * gradientFac), 2));
            int add = static_cast<int>(f * 255);
            int ng = std::clamp(gray + add, 0, 255);
            result.setPixel(x, y, qRgb(ng, ng, ng));
        }
    }
    return result;
}
