// LightPollution.h
#ifndef LIGHTPOLLUTION_H
#define LIGHTPOLLUTION_H

#include <QImage>

class LightPollution {
public:
    // image              — входное (размытое) изображение
    // centerX, centerY   — центр солнца в пикселях
    // baseIntensity      — яркость центрального ореола [0..1]
    // baseRadiusFactor   — радиус ореола = max(W,H)*factor
    // numRays            — число тонких «лучей»
    // rayIntensity       — яркость у основания каждого луча [0..1]
    // maxRayLengthFactor — макс. длина лучей = max(W,H)*factor
    static QImage applySunFlare(
        const QImage& image,
        double centerX,
        double centerY,
        double baseIntensity      = 0.2,
        double baseRadiusFactor   = 0.6,
        int    numRays            = 32,
        double rayIntensity       = 0.5,
        double maxRayLengthFactor = 1.0,
        double coreRadius         = 45
        );
};

#endif // LIGHTPOLLUTION_H
