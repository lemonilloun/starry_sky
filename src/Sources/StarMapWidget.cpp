#include "StarMapWidget.h"
#include "GaussianBlur.h"
#include "LightPollution.h"
#include <QPainter>
#include <QPaintEvent>
#include <cmath>
#include <limits>

StarMapWidget::StarMapWidget(
    const std::vector<double> &x,
    const std::vector<double> &y,
    const std::vector<double> &mag,
    QWidget *parent
    ) : QWidget(parent),
    xCoords(x),
    yCoords(y),
    magnitudes(mag)
{
    starMapImage = QImage(1081, 761, QImage::Format_Grayscale8);
    starMapImage.fill(Qt::black);

    renderStars();
    blurredImage = GaussianBlur::applyGaussianBlur(starMapImage, 13, 2.0, 1.0, 0.60);
}

void StarMapWidget::setSunPosition(double xi, double eta) {
    sunXi  = xi;
    sunEta = eta;
}

void StarMapWidget::renderStars() {
    QPainter p(&starMapImage);
    p.setRenderHint(QPainter::Antialiasing);

    double minX = std::numeric_limits<double>::max(),
        maxX = std::numeric_limits<double>::lowest(),
        minY = std::numeric_limits<double>::max(),
        maxY = std::numeric_limits<double>::lowest();
    for (size_t i = 0; i < xCoords.size(); ++i) {
        minX = std::min(minX, xCoords[i]);
        maxX = std::max(maxX, xCoords[i]);
        minY = std::min(minY, yCoords[i]);
        maxY = std::max(maxY, yCoords[i]);
    }
    if (xCoords.empty() || minX == maxX || minY == maxY) {
        p.drawText(starMapImage.rect(), Qt::AlignCenter, "No stars to display");
        return;
    }

    int W = starMapImage.width(), H = starMapImage.height();
    double widthBBox  = maxX - minX,
        heightBBox = maxY - minY;
    double scale = std::min(W / widthBBox, H / heightBBox);
    double offsetX = -minX, offsetY = -minY;
    double marginX = (W - widthBBox * scale) / 2.0;
    double marginY = (H - heightBBox * scale) / 2.0;

    const double brightnessScale = 15.0;
    const int    minBrightness   = 60;
    const double maxStarRadius   = 5.0;

    for (size_t i = 0; i < xCoords.size(); ++i) {
        double sx = marginX + (xCoords[i] + offsetX) * scale;
        double sy = marginY + (yCoords[i] + offsetY) * scale;
        sy = H - sy;  // invert Y

        double bf = brightnessScale / std::pow(2.512, magnitudes[i]);
        int    br = std::max(minBrightness, int(std::clamp(bf * 255.0, 0.0, 255.0)));
        double r  = std::clamp(1.5 * std::sqrt(bf), 1.0, maxStarRadius);

        QColor c(br, br, br);
        p.setPen(c);
        p.setBrush(c);
        p.drawEllipse(QPointF(sx, sy), r, r);
    }
}

void StarMapWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);

    // 1) сначала обычное размытие
    painter.drawImage(rect(), blurredImage);

    // 2) теперь flare от Солнца
    int W = starMapImage.width(), H = starMapImage.height();

    // повторяем те же bbox/scale, чтобы из ξ,η получить пиксели:
    double minX = std::numeric_limits<double>::max(),
        maxX = std::numeric_limits<double>::lowest(),
        minY = std::numeric_limits<double>::max(),
        maxY = std::numeric_limits<double>::lowest();
    for (size_t i = 0; i < xCoords.size(); ++i) {
        minX = std::min(minX, xCoords[i]);
        maxX = std::max(maxX, xCoords[i]);
        minY = std::min(minY, yCoords[i]);
        maxY = std::max(maxY, yCoords[i]);
    }
    double widthBBox  = maxX - minX,
        heightBBox = maxY - minY;
    double scale = std::min(W / widthBBox, H / heightBBox);
    double offsetX = -minX, offsetY = -minY;
    double marginX = (W - widthBBox * scale) / 2.0;
    double marginY = (H - heightBBox * scale) / 2.0;

    double sunX = marginX + (sunXi + offsetX) * scale;
    double sunY = marginY + (sunEta + offsetY) * scale;
    sunY = H - sunY;

    QImage withFlare = LightPollution::applySunFlare(
        blurredImage,
        sunX, sunY,
        /*intensity=*/0.3,
        /*gradientFactor=*/0.8
        );

    // 3) выводим итог
    painter.drawImage(rect(), withFlare);
}
