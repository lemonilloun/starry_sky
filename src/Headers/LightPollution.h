// LightPollution.h  (в каталоге src/Utils)
#ifndef LIGHTPOLLUTION_H
#define LIGHTPOLLUTION_H

#include <QImage>

namespace LightPollution {
QImage applySunFlare(
    const QImage &image,
    double centerX,
    double centerY,
    double intensity = 0.2,
    double gradientFac = 0.8
    );
}

#endif // LIGHTPOLLUTION_H
