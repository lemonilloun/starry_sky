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
    QWidget*                     parent
    ) : QWidget(parent),
    xCoords(x),
    yCoords(y),
    magnitudes(m),
    starIds(ids),
    sun(sunInfo)
{
    // 1) black canvas
    starMapImage = QImage(1081, 761, QImage::Format_Grayscale8);
    starMapImage.fill(Qt::black);

    // 2) draw all stars (including the Sun‐disk only if it's in‐frame)
    renderStars();

    // 3) Gaussian blur
    blurredImage = GaussianBlur::applyGaussianBlur(
        starMapImage,
        /*kernelSize=*/13,
        /*sigmaX=*/2.0,
        /*sigmaY=*/1.0,
        /*rho=*/0.75
        );

    // 4) if the Sun was projected (sun.apply==true), overlay a
    //    shrinking/fading flare as it drifts out of frame
    if (sun.apply) {
        int W = blurredImage.width();
        int H = blurredImage.height();

        // recompute the same bbox & scale as in renderStars()
        double minX = +1e9, maxX = -1e9, minY = +1e9, maxY = -1e9;
        for (double v : xCoords) minX = std::min(minX, v), maxX = std::max(maxX, v);
        for (double v : yCoords) minY = std::min(minY, v), maxY = std::max(maxY, v);
        double dx = maxX - minX, dy = maxY - minY;
        if (dx > 0 && dy > 0) {
            double cx    = 0.5*(minX + maxX);
            double cy    = 0.5*(minY + maxY);
            double scale = std::min(W/dx, H/dy);

            // pixel coords of Sun center
            double sunXpix = W/2.0 + (sun.xi - cx)*scale;
            double sunYpix = H/2.0 - (sun.eta - cy)*scale;

            // full halo radius in px (baseRadiusFactor==1.0)
            double Rpix_full = std::max(W,H) * 1.0;

            // distance of Sun center from frame center
            double ddx = sunXpix - W/2.0;
            double ddy = sunYpix - H/2.0;
            double distCenter = std::sqrt(ddx*ddx + ddy*ddy);

            // only draw as long as some of the halo overlaps
            double overlap = Rpix_full - distCenter;
            if (overlap > 0) {
                double frac = overlap / Rpix_full;  // 1.0 inside frame → 0 as it drifts out

                // shrink both radius & intensity by frac
                double effRadiusFactor = frac * 1.0;        // baseRadiusFactor
                double effIntensity    = frac * 0.4;        // baseIntensity

                blurredImage = LightPollution::applySunFlare(
                    blurredImage,
                    sunXpix,
                    sunYpix,
                    /*baseIntensity=*/     effIntensity,
                    /*baseRadiusFactor=*/  effRadiusFactor,
                    /*numRays=*/           128,
                    /*rayIntensity=*/      0.2,
                    /*maxRayLengthFactor=*/0.3,
                    /*coreRadius=*/        52.0
                    );
            }
        }
    }

    // 5) global gentle boost & noise so the sky isn’t pitch-black
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
            double r  = std::clamp(1.5*std::sqrt(bf), 1.0, 4.0);

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
