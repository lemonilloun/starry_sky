// StarMapWidget.cpp
#include "StarMapWidget.h"
#include "GaussianBlur.h"
#include "LightPollution.h"
#include <iostream>
#include <QPainter>
#include <limits>
#include <algorithm>
#include <cmath>

static constexpr uint64_t SUN_ID = 1010101010; // ваш уникальный ID

StarMapWidget::StarMapWidget(
    const std::vector<double>&   x,
    const std::vector<double>&   y,
    const std::vector<double>&   m,
    const std::vector<uint64_t>& ids,
    const StarCatalog::Sun&      sunInfo,
    const BlurParams            blurParams,
    bool                         blurEnabled,
    const FlareParams           flareParams,
    bool                         flareEnabled,

    QWidget*                     parent
    ) : QWidget(parent),
    xCoords(x),
    yCoords(y),
    magnitudes(m),
    starIds(ids),
    sun(sunInfo),
    blurParams(blurParams),
    m_blurEnabled(blurEnabled),
    flareParams(flareParams),
    m_flareEnabled(flareEnabled)
{
    // 1) чёрно-белый холст
    starMapImage = QImage(1081, 761, QImage::Format_Grayscale8);
    starMapImage.fill(Qt::black);

    // 2) рисуем все точки (звёзды + диск Солнца, если он в кадре)
    renderStars();

    // 3) размытие Гауссом
    if (m_blurEnabled) {
        blurredImage = GaussianBlur::applyGaussianBlur(
            starMapImage,
            /*kernelSize=*/blurParams.kernelSize,
            /*sigmaX=*/blurParams.sigmaX,
            /*sigmaY=*/blurParams.sigmaY,
            /*rho=*/blurParams.rho
            );
    } else {
        blurredImage = starMapImage;
    }

    // 4) если Солнце «попало» (sun.apply==true), рисуем flare,
    if (m_flareEnabled && sun.apply) {
        int W = blurredImage.width();
        int H = blurredImage.height();

        // пересчитаем bbox & scale из renderStars()
        double minX = +1e9, maxX = -1e9, minY = +1e9, maxY = -1e9;
        for (double v : xCoords) minX = std::min(minX, v), maxX = std::max(maxX, v);
        for (double v : yCoords) minY = std::min(minY, v), maxY = std::max(maxY, v);
        double dx = maxX - minX, dy = maxY - minY;
        if (dx > 0 && dy > 0) {
            double cx    = 0.5*(minX + maxX);
            double cy    = 0.5*(minY + maxY);
            double scale = std::min(W/dx, H/dy);

            // координаты Солнца в пикселях
            double sunXpix = W/2.0 + (sun.xi  - cx)*scale;
            double sunYpix = H/2.0 - (sun.eta - cy)*scale;

            // полный радиус ореола (baseRadiusFactor == 1.0)
            double Rpix_full = std::max(W, H) * 1.5;

            // расстояние центра flare до центра кадра
            double ddx = sunXpix - W/2.0;
            double ddy = sunYpix - H/2.0;
            double distCenter = std::sqrt(ddx*ddx + ddy*ddy);

            // сколько «облака» ещё перекрывает кадр
            double overlap = Rpix_full - distCenter;
            if (overlap > 0) {
                // доля полного ореола, которая ещё внутри кадра
                double frac = overlap / Rpix_full;  // 1.0 в центре → 0 по краю

                // уменьшаем и радиус, и яркость пропорционально frac
                double effRadiusFactor = frac * flareParams.baseRadiusFactor;  // чуть «раздуваем» чуть-чуть
                double effIntensity    = frac * flareParams.baseIntensity;  // базовая яркость

                blurredImage = LightPollution::applySunFlare(
                    blurredImage,
                    sunXpix,
                    sunYpix,
                    /*baseIntensity=*/     effIntensity,
                    /*baseRadiusFactor=*/  effRadiusFactor,
                    /*numRays=*/           flareParams.numRays,
                    /*rayIntensity=*/      flareParams.rayIntensity,
                    /*maxRayLengthFactor=*/flareParams.maxRayLengthFactor,
                    /*coreRadius=*/        flareParams.coreRadius
                    );
            }
        }
    }

    // 5) лёгкий глобальный «буст» и шум, чтобы не было чисто-чёрного фона
    blurredImage = LightPollution::applyGlobalBoost(
        blurredImage,
        /*boost=*/          0.1,
        /*noiseAmplitude=*/ 0.01
        );
}

void StarMapWidget::renderStars()
{
    QPainter p(&starMapImage);
    p.setRenderHint(QPainter::Antialiasing);

    const int W = starMapImage.width();
    const int H = starMapImage.height();

    // 1) bbox всех точек
    double minX = +1e9, maxX = -1e9, minY = +1e9, maxY = -1e9;
    for (double v : xCoords) minX = std::min(minX, v), maxX = std::max(maxX, v);
    for (double v : yCoords) minY = std::min(minY, v), maxY = std::max(maxY, v);
    double dx = maxX - minX, dy = maxY - minY;
    if (dx <= 0 || dy <= 0) return;

    double cx    = 0.5*(minX + maxX);
    double cy    = 0.5*(minY + maxY);
    double scale = std::min(W/dx, H/dy);

    const double starSizeFactor = 2.0;

    // 2) рисуем все точки
    for (size_t i = 0; i < xCoords.size(); ++i) {
        double x = xCoords[i], y = yCoords[i], m = magnitudes[i];
        uint64_t id = (i < starIds.size() ? starIds[i] : 0ULL);

        double sx = W/2.0 + (x - cx)*scale;
        double sy = H/2.0 - (y - cy)*scale;

        if (id == SUN_ID) {
            // сам диск солнца чуть больше и жёлтый
            QColor sunCol(255, 230, 150);
            p.setPen(Qt::NoPen);
            p.setBrush(sunCol);
            p.drawEllipse(QPointF(sx, sy), 48.0, 53.0);
        } else {
            // обычная звезда
            double bf = 27.0 / std::pow(2.512, m);
            int    br = std::clamp(int(bf*255.0), 40, 255);
            double baseR = std::clamp(1.5*std::sqrt(bf), 1.0, 4.0);
            double r     = baseR * starSizeFactor;

            QColor col(br, br, br);
            p.setPen(Qt::NoPen);
            p.setBrush(col);
            p.drawEllipse(QPointF(sx, sy), r, r);
        }
    }
}

void StarMapWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.drawImage(rect(), blurredImage);
}
