// StarMapWidget.cpp
#include "StarMapWidget.h"
#include "GaussianBlur.h"
#include "LightPollution.h"

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
    QWidget* parent
    ) : QWidget(parent),
    xCoords(x), yCoords(y),
    magnitudes(m),
    starIds(ids)
{
    // 1) Чёрно-белое полотно
    starMapImage = QImage(1081, 761, QImage::Format_Grayscale8);
    starMapImage.fill(Qt::black);

    // 2) Рисуем все звёзды и Солнце как точку
    renderStars();

    // 3) Размываем Гауссом
    blurredImage = GaussianBlur::applyGaussianBlur(
        starMapImage,
        /*kernelSize=*/13,
        /*sigmaX=*/2.0,
        /*sigmaY=*/1.0,
        /*rho=*/0.75
        );

    // 4) Если Солнце в списке и попало в кадр — добавляем flare
    int W = blurredImage.width();
    int H = blurredImage.height();

    // снова вычислим bbox/scale, чтобы найти пиксельные координаты солнца:
    double minX = +1e9, maxX = -1e9, minY = +1e9, maxY = -1e9;
    for (double v : xCoords) minX = std::min(minX, v), maxX = std::max(maxX, v);
    for (double v : yCoords) minY = std::min(minY, v), maxY = std::max(maxY, v);
    double dx = maxX-minX, dy = maxY-minY;
    if (dx>0 && dy>0) {
        double cx    = 0.5*(minX+maxX);
        double cy    = 0.5*(minY+maxY);
        double scale = std::min(W/dx, H/dy);

        for (size_t i = 0; i < starIds.size(); ++i) {
            if (starIds[i] == SUN_ID) {
                double sunXpix = W/2.0 + (xCoords[i] - cx)*scale;
                double sunYpix = H/2.0 - (yCoords[i] - cy)*scale;

                // рисуем flare с радиусом 40% от большего размера
                //double R = 0.6 * std::max(W, H);
                blurredImage = LightPollution::applySunFlare(
                    blurredImage,
                    sunXpix, sunYpix,
                    /*baseIntensity=*/0.4,
                    /*baseRadiusFactor=*/1.0,
                    /*numRays=*/128,
                    /*rayIntensity=*/0.2,
                    /*maxRayLengthFactor=*/0.3,
                    /*coreRadius=*/52.0
                    );
                break;
            }
        }
    }

    blurredImage = LightPollution::applyGlobalBoost(
        blurredImage,
        /*boost*/         0.1,
        /*noiseAmplitude*/0.01
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
