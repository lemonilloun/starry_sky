#ifndef LIGHTPOLLUTION_H
#define LIGHTPOLLUTION_H

#include <QImage>

class LightPollution {
public:
    // Применение фильтра светового загрязнения к изображению
    static QImage applyLightPollution(const QImage& image, double intensity = 1.0, double gradientFactor = 2.0);
};

#endif // LIGHTPOLLUTION_H
