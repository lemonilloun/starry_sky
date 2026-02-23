// LightPollution.cpp

#include "LightPollution.h"
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QRandomGenerator>
#include <cmath>

QImage LightPollution::applySunFlare(
    const QImage& image,
    double centerX,
    double centerY,
    double baseIntensity,     // [0..1] яркость ореола
    double baseRadiusFactor,  // радиус ореола = max(W,H)*baseRadiusFactor
    int    numRays,           // число дополнительных лучей
    double rayIntensity,      // [0..1] яркость начала луча
    double maxRayLengthFactor,// макс. длина луча = max(W,H)*factor
    double coreRadius         // радиус твёрдого ядра
    ) {
    int W = image.width();
    int H = image.height();
    QImage result = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QPainter p(&result);
    p.setRenderHint(QPainter::Antialiasing);

    // 1) Центральный радиальный ореол
    double R = std::max(W, H) * baseRadiusFactor;
    QRadialGradient rg(QPointF(centerX, centerY), R);
    rg.setColorAt(coreRadius/R, QColor(255,255,255, int(baseIntensity*255)));
    rg.setColorAt(1.0, QColor(0,0,0,0));
    p.setBrush(rg);
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(centerX, centerY), R, R);

    // 2) Три «основных» луча и их зеркальные отростки
    double axisAng = QRandomGenerator::global()->generateDouble() * 2*M_PI;
    double spread  = 0.05;  // ±6°
    double offsets[3] = {-spread, 0.0, +spread};
    double mainLenF = 1.2;       // длина основного луча
    double sideLenF = 0.8;       // длина боковых лучей
    double maxLen   = std::max(W,H) * maxRayLengthFactor;

    // для каждого из 3-х смещений и для зеркала
    for (int i=0; i<3; ++i) {
        for (int mir=0; mir<2; ++mir) {
            double ang = axisAng + offsets[i] + (mir ? M_PI : 0.0);
            double dx =  cos(ang), dy = -sin(ang);
            double lenF = (i==1 ? mainLenF : sideLenF);
            QPointF start(centerX + dx*coreRadius,
                          centerY + dy*coreRadius);
            QPointF end(centerX + dx*(coreRadius + lenF*maxLen),
                        centerY + dy*(coreRadius + lenF*maxLen));

            QLinearGradient lg(start, end);
            lg.setColorAt(0.0, QColor(255,255,255, int(rayIntensity*255)));
            lg.setColorAt(1.0, QColor(255,255,255,   0));

            // ширина основных лучей ≥6px, +0..3px рандом
            int w = 10 + QRandomGenerator::global()->bounded(4);
            QPen pen(lg, w, Qt::SolidLine, Qt::RoundCap);
            p.setPen(pen);
            p.drawLine(start, end);
        }
    }

    // 3) Дополнительные лучи, равномерно по окружности
    for (int i=0; i<numRays; ++i) {
        double ang = (2*M_PI * i) / numRays;
        double dx =  cos(ang), dy = -sin(ang);
        double len = coreRadius + maxLen * (0.5 + QRandomGenerator::global()->generateDouble()*0.5);

        QPointF start(centerX + dx*coreRadius,
                      centerY + dy*coreRadius);
        QPointF end(centerX + dx*len,
                    centerY + dy*len);

        QLinearGradient lg(start, end);
        lg.setColorAt(0.0, QColor(255,255,255, int(rayIntensity*200))); // чуть более мягко
        lg.setColorAt(1.0, QColor(255,255,255,    0));

        // тонкие дополнительные лучи: 2px
        QPen pen(lg, 2, Qt::SolidLine, Qt::RoundCap);
        p.setPen(pen);
        p.drawLine(start, end);
    }

    double crossBase = axisAng + M_PI_2; // поворот на 90°
    double crossOffsets[3] = {-spread, 0.0, +spread};
    for (int i=0; i<3; ++i) {
        for (int mir=0; mir<2; ++mir) {
            double ang = crossBase + crossOffsets[i] + (mir ? M_PI : 0.0);
            double dx =  std::cos(ang), dy = -std::sin(ang);
            // для «крестовых» лучей даём чуть больше рандома по длине
            double lenF = (i==1 ? (mainLenF * (0.8 + QRandomGenerator::global()->generateDouble()*0.2))
                                  : (sideLenF * (0.8 + QRandomGenerator::global()->generateDouble()*0.2)));
            QPointF start(centerX + dx*coreRadius,
                          centerY + dy*coreRadius);
            QPointF end(centerX + dx*(coreRadius + lenF*maxLen),
                        centerY + dy*(coreRadius + lenF*maxLen));

            QLinearGradient lg(start, end);
            // чуть меньшая интенсивность для крестовых лучей
            lg.setColorAt(0.0, QColor(255,255,255, int(rayIntensity*200)));
            lg.setColorAt(1.0, QColor(255,255,255,   0));

            int w = 10 + QRandomGenerator::global()->bounded(4);
            QPen pen(lg, w, Qt::SolidLine, Qt::RoundCap);
            p.setPen(pen);
            p.drawLine(start, end);
        }
    }

    // 3) Шумной контур «ядра» солнца
    const int M = 64;             // число вершин шумного контура
    const double noiseAmp = 0.05; // амплитуда шума ±5%
    QPainterPath noisyCore;
    // стартовая точка на углу 0
    noisyCore.moveTo(centerX + coreRadius, centerY);
    for (int k = 1; k <= M; ++k) {
        double ang = (2*M_PI*k) / double(M);
        double n   = (QRandomGenerator::global()->generateDouble()*2 - 1) * noiseAmp;
        double r   = coreRadius * (1.0 + n);
        double px  = centerX + std::cos(ang) * r;
        double py  = centerY - std::sin(ang) * r;
        noisyCore.lineTo(px, py);
    }
    noisyCore.closeSubpath();

    p.setBrush(QColor(248, 244, 232, 200)); // более нейтральный цвет ядра
    p.setPen(Qt::NoPen);
    p.drawPath(noisyCore);

    return result;
}

QImage LightPollution::applyGlobalBoost(
    const QImage& image,
    double boost,
    double noiseAmplitude
    ) {
    int W = image.width();
    int H = image.height();
    QImage result = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);

    for (int y = 0; y < H; ++y) {
        auto* scan = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < W; ++x) {
            QRgb px = scan[x];
            const int a = qAlpha(px);
            const double noise = (QRandomGenerator::global()->generateDouble() * 2.0 - 1.0)
                               * noiseAmplitude * 255.0;
            const double add = boost * 255.0 + noise;

            int r = qBound(0, std::lround(qRed(px) + add), 255);
            int g = qBound(0, std::lround(qGreen(px) + add), 255);
            int b = qBound(0, std::lround(qBlue(px) + add), 255);
            scan[x] = qRgba(r, g, b, a);
        }
    }

    return result;
}
