// StarMapWidget.cpp
#include "StarMapWidget.h"
#include "GaussianBlur.h"
#include "LightPollution.h"
#include <iostream>
#include <QPainter>
#include <QLabel>
#include <QVBoxLayout>
#include <QStringList>
#include <limits>
#include <algorithm>
#include <utility>
#include <cmath>

static constexpr uint64_t SUN_ID = 1010101010; // ваш уникальный ID

namespace {
QString formatRightAscension(double raRad)
{
    double hours = raRad * 12.0 / M_PI;
    hours = std::fmod(hours, 24.0);
    if (hours < 0.0)
        hours += 24.0;

    int h = static_cast<int>(std::floor(hours));
    double minutesF = (hours - h) * 60.0;
    int m = static_cast<int>(std::floor(minutesF));
    double seconds = (minutesF - m) * 60.0;

    return QString("%1h %2m %3.1fs")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(seconds, 4, 'f', 1, QChar('0'));
}

QString formatDeclination(double decRad)
{
    double degrees = decRad * 180.0 / M_PI;
    double absDeg = std::fabs(degrees);
    int d = static_cast<int>(std::floor(absDeg));
    double minutesF = (absDeg - d) * 60.0;
    int m = static_cast<int>(std::floor(minutesF));
    double seconds = (minutesF - m) * 60.0;
    QChar sign = degrees >= 0.0 ? QChar('+') : QChar('-');

    return QString("%1%2 deg %3' %4.0\"")
        .arg(sign)
        .arg(d, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(seconds, 2, 'f', 0, QChar('0'));
}
} // namespace

StarMapWidget::StarMapWidget(
    std::vector<StarProjection> projections,
    const StarCatalog::Sun&      sunInfo,
    const BlurParams             blurParams,
    bool                         blurEnabled,
    const FlareParams            flareParams,
    bool                         flareEnabled,
    QWidget*                     parent
    )
    : QWidget(parent),
      m_projections(std::move(projections)),
      m_pixelPositions(m_projections.size()),
      sun(sunInfo),
      blurParams(blurParams),
      m_blurEnabled(blurEnabled),
      flareParams(flareParams),
      m_flareEnabled(flareEnabled),
      m_infoPopup(nullptr),
      m_infoLabel(nullptr)
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
    if (m_flareEnabled && sun.apply && m_hasGeometry) {
        int W = blurredImage.width();
        int H = blurredImage.height();

        double sunXpix = W / 2.0 + (sun.xi  - m_centerXi) * m_scale;
        double sunYpix = H / 2.0 - (sun.eta - m_centerEta) * m_scale;

        double Rpix_full = std::max(W, H) * 1.5;

        double ddx = sunXpix - W/2.0;
        double ddy = sunYpix - H/2.0;
        double distCenter = std::sqrt(ddx*ddx + ddy*ddy);

        double overlap = Rpix_full - distCenter;
        if (overlap > 0) {
            double frac = overlap / Rpix_full;
            double effRadiusFactor = frac * flareParams.baseRadiusFactor;
            double effIntensity    = frac * flareParams.baseIntensity;

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

    if (m_projections.empty()) {
        m_pixelPositions.clear();
        m_hasGeometry = false;
        return;
    }

    double minX = std::numeric_limits<double>::max();
    double maxX = -std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();
    double maxY = -std::numeric_limits<double>::max();

    for (const auto& proj : m_projections) {
        minX = std::min(minX, proj.x);
        maxX = std::max(maxX, proj.x);
        minY = std::min(minY, proj.y);
        maxY = std::max(maxY, proj.y);
    }

    double dx = maxX - minX;
    double dy = maxY - minY;
    if (dx <= 0.0 || dy <= 0.0) {
        m_pixelPositions.clear();
        m_hasGeometry = false;
        return;
    }

    m_minXi = minX;
    m_maxXi = maxX;
    m_minEta = minY;
    m_maxEta = maxY;
    m_centerXi  = 0.5 * (minX + maxX);
    m_centerEta = 0.5 * (minY + maxY);
    m_scale     = std::min(W / dx, H / dy);
    m_hasGeometry = true;

    const double starSizeFactor = 2.0;

    m_pixelPositions.resize(m_projections.size());

    // 2) рисуем все точки
    for (size_t i = 0; i < m_projections.size(); ++i) {
        const auto& proj = m_projections[i];
        double x = proj.x;
        double y = proj.y;
        double m = proj.magnitude;
        uint64_t id = static_cast<uint64_t>(proj.starId);

        double sx = W / 2.0 + (x - m_centerXi) * m_scale;
        double sy = H / 2.0 - (y - m_centerEta) * m_scale;
        m_pixelPositions[i] = QPointF(sx, sy);

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

void StarMapWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_hasGeometry) {
        QPointF clicked = event->position();
        const double threshold = 6.0;
        const double threshold2 = threshold * threshold;

        int foundIndex = -1;
        double bestDist2 = threshold2;

        for (int i = 0; i < static_cast<int>(m_pixelPositions.size()); ++i) {
            const QPointF& starPos = m_pixelPositions[i];
            double dx = starPos.x() - clicked.x();
            double dy = starPos.y() - clicked.y();
            double dist2 = dx*dx + dy*dy;
            if (dist2 <= bestDist2) {
                bestDist2 = dist2;
                foundIndex = i;
            }
        }

        if (foundIndex >= 0 && foundIndex < static_cast<int>(m_projections.size())) {
            QString infoText = formatStarInfo(m_projections[foundIndex]);
            showInfoPopup(event->globalPosition().toPoint(), infoText);
        } else {
            hideInfoPopup();
        }
    } else if (event->button() == Qt::LeftButton) {
        hideInfoPopup();
    }

    QWidget::mousePressEvent(event);
}

QString StarMapWidget::formatStarInfo(const StarProjection& projection) const
{
    const QString name = QString::fromStdString(projection.displayName);
    const bool isCatalogName = name.startsWith("HIP ")
                            || name.startsWith("HD ")
                            || name.startsWith("HR ")
                            || name.startsWith("Star ");

    auto fmt = [](const QString& label, const QString& value) {
        return QString("<span style='font-weight:600;'>%1:</span> %2")
            .arg(label.toHtmlEscaped())
            .arg(value.toHtmlEscaped());
    };

    QStringList infoLines;
    infoLines << fmt(QString::fromUtf8("α"), formatRightAscension(projection.raRad));
    infoLines << fmt(QString::fromUtf8("δ"), formatDeclination(projection.decRad));
    infoLines << fmt("Mag", QString::number(projection.magnitude, 'f', 2));

    for (const auto& entry : projection.catalogDesignations) {
        if (entry.empty())
            continue;
        const auto colonPos = entry.find(' ');
        QString label, value;
        if (colonPos != std::string::npos) {
            label = QString::fromStdString(entry.substr(0, colonPos));
            value = QString::fromStdString(entry.substr(colonPos + 1));
        } else {
            label = QStringLiteral("ID");
            value = QString::fromStdString(entry);
        }
        infoLines << fmt(label, value);
    }

    QString html;
    if (!name.isEmpty()) {
        if (isCatalogName) {
            html += name.toHtmlEscaped();
        } else {
            html += QString("<span style='font-style:italic;'>%1</span>")
                    .arg(name.toHtmlEscaped());
        }
    }

    if (!infoLines.isEmpty()) {
        if (!html.isEmpty())
            html += "<hr style='border:0;border-top:1px solid rgba(255,255,255,0.35);margin:0;'>";
        html += infoLines.join("<br>");
    }

    return html;
}

void StarMapWidget::showInfoPopup(const QPoint& globalPos, const QString& text)
{
    if (!m_infoPopup) {
        m_infoPopup = new QWidget(this, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        m_infoPopup->setAttribute(Qt::WA_ShowWithoutActivating);
        m_infoPopup->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_infoPopup->setFocusPolicy(Qt::NoFocus);
        m_infoPopup->setStyleSheet(
            "background-color: rgba(24,24,24,220);"
            "color: #f0f0f0;"
            "border: 1px solid rgba(220,220,220,180);"
            "border-radius: 6px;"
        );

        auto *layout = new QVBoxLayout(m_infoPopup);
        layout->setContentsMargins(12, 10, 12, 10);
        m_infoLabel = new QLabel(m_infoPopup);
        m_infoLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_infoLabel->setWordWrap(true);
        m_infoLabel->setTextFormat(Qt::RichText);
        m_infoLabel->setStyleSheet(
            "QLabel {"
            "  font-family: 'Inter', 'Helvetica', 'Arial', sans-serif;"
            "  font-size: 14px;"
            "  color: #f2f5ff;"
            "}"
        );
        layout->addWidget(m_infoLabel);
    }

    if (!m_infoLabel)
        return;

    m_infoLabel->setText(text);
    m_infoPopup->adjustSize();
    QPoint offset(14, 14);
    m_infoPopup->move(globalPos + offset);
    m_infoPopup->show();
    m_infoPopup->raise();
}

void StarMapWidget::hideInfoPopup()
{
    if (m_infoPopup)
        m_infoPopup->hide();
}
