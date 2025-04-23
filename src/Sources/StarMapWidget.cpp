#include "StarMapWidget.h"
#include "GaussianBlur.h"

#include <QPainter>
#include <limits>
#include <algorithm>
#include <cmath>

StarMapWidget::StarMapWidget(
    const std::vector<double>& x,
    const std::vector<double>& y,
    const std::vector<double>& m,
    const std::vector<uint64_t>& ids,
    QWidget* parent
    ) : QWidget(parent),
    xCoords(x),
    yCoords(y),
    magnitudes(m),
    starIds(ids)
{
    // 1) Создаем изображение чёрно-белое
    starMapImage = QImage(1081, 761, QImage::Format_Grayscale8);
    starMapImage.fill(Qt::black);

    // 2) Рисуем все точки (вместе с «Солнцем» по ID)
    renderStars();

    // 3) Применяем Гаусс-размытие
    blurredImage = GaussianBlur::applyGaussianBlur(
        starMapImage,
        /*kernelSize=*/13,
        /*sigmaX=*/2.0,
        /*sigmaY=*/1.0,
        /*rho=*/0.75
        );
}

void StarMapWidget::renderStars()
{
    QPainter p(&starMapImage);
    p.setRenderHint(QPainter::Antialiasing);

    const int W = starMapImage.width();
    const int H = starMapImage.height();

    // 1) Считаем bounding-box
    double minX =  std::numeric_limits<double>::infinity();
    double maxX = -minX;
    double minY =  minX;
    double maxY = -minX;

    for (double xx : xCoords) { minX = std::min(minX, xx); maxX = std::max(maxX, xx); }
    for (double yy : yCoords) { minY = std::min(minY, yy); maxY = std::max(maxY, yy); }
    double dx = maxX - minX, dy = maxY - minY;
    if (dx <= 0 || dy <= 0) return;

    double cx    = 0.5 * (minX + maxX);
    double cy    = 0.5 * (minY + maxY);
    double scale = std::min(W/dx, H/dy);

    // 2) Рисуем
    for (size_t i = 0; i < xCoords.size(); ++i) {
        double x = xCoords[i];
        double y = yCoords[i];
        double m = magnitudes[i];
        uint64_t id = (i < starIds.size() ? starIds[i] : 0ULL);

        double sx = W/2.0 + (x - cx) * scale;
        double sy = H/2.0 - (y - cy) * scale;

        // наш «солнечный» ID:
        if (id == 1010101010) {
            QColor sunCol(255, 230, 150);
            p.setPen(Qt::NoPen);
            p.setBrush(sunCol);
            p.drawEllipse(QPointF(sx, sy), 10.0, 10.0);
            continue;
        }

        // обычная звезда
        const double brightnessScale = 27.0;
        const int    minBrightness   = 60;
        const double maxStarRadius   = 5.0;

        double bf  = brightnessScale / std::pow(2.512, m);
        int    brt = std::clamp(int(bf * 255.0), minBrightness, 255);
        double r   = std::clamp(1.5 * std::sqrt(bf), 1.0, maxStarRadius);

        QColor col(brt, brt, brt);
        p.setPen(Qt::NoPen);
        p.setBrush(col);
        p.drawEllipse(QPointF(sx, sy), r, r);
    }
}

void StarMapWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.drawImage(rect(), blurredImage);
}
