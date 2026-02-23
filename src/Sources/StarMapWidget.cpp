// StarMapWidget.cpp
#include "StarMapWidget.h"
#include "CelestialBodyTypes.h"
#include "GaussianBlur.h"
#include "LightPollution.h"
#include <iostream>
#include <QPainter>
#include <QPainterPath>
#include <QLabel>
#include <QVBoxLayout>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QStringList>
#include <limits>
#include <algorithm>
#include <utility>
#include <cmath>

static constexpr uint64_t SUN_ID = astro::bodyIdValue(astro::BodyId::Sun);
static constexpr uint64_t MOON_ID = astro::bodyIdValue(astro::BodyId::Moon);
static constexpr uint64_t VENUS_ID = astro::bodyIdValue(astro::BodyId::Venus);
static constexpr uint64_t MARS_ID = astro::bodyIdValue(astro::BodyId::Mars);

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
    ObservationInfo              observation,
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
      m_infoLabel(nullptr),
      m_observation(observation)
{
    setFocusPolicy(Qt::StrongFocus);
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
    const double sunVisualRadiusPx = 53.0;
    const double sunVisualDiamPx = 2.0 * sunVisualRadiusPx;
    const double defaultSunAngularDiamRad = 0.533 * M_PI / 180.0;

    double sunAngularDiamRad = defaultSunAngularDiamRad;
    QPointF sunPix;
    bool hasSunPix = false;

    for (const auto& proj : m_projections) {
        const uint64_t id = static_cast<uint64_t>(proj.starId);
        if (id != SUN_ID)
            continue;
        if (proj.angularDiameterRad > 0.0)
            sunAngularDiamRad = proj.angularDiameterRad;
        const double sx = W / 2.0 + (proj.x - m_centerXi) * m_scale;
        const double sy = H / 2.0 - (proj.y - m_centerEta) * m_scale;
        sunPix = QPointF(sx, sy);
        hasSunPix = true;
        break;
    }

    m_pixelPositions.resize(m_projections.size());
    m_pixelRadii.resize(m_projections.size());

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
            p.drawEllipse(QPointF(sx, sy), 48.0, sunVisualRadiusPx);
            m_pixelRadii[i] = sunVisualRadiusPx;
        } else if (id == MOON_ID) {
            const double moonAngularDiamRad = (proj.angularDiameterRad > 0.0)
                ? proj.angularDiameterRad
                : (0.52 * M_PI / 180.0);

            const double ratio = moonAngularDiamRad / std::max(1e-12, sunAngularDiamRad);
            const double styleScale = 0.96;
            const double moonDiamStyled = sunVisualDiamPx * ratio * styleScale;
            const double moonDiamPhys = 2.0 * m_scale * std::tan(0.5 * moonAngularDiamRad);
            double moonDiamPx = 0.80 * moonDiamStyled + 0.20 * moonDiamPhys;
            moonDiamPx = std::clamp(moonDiamPx, 0.82 * sunVisualDiamPx, 1.02 * sunVisualDiamPx);
            const double r = 0.5 * moonDiamPx;
            const double glowR = r * 1.9;
            const double illumination = std::clamp(proj.illumination, 0.0, 1.0);

            // Soft moon glow so the Moon stands out as a special body.
            QRadialGradient glowGrad(QPointF(sx, sy), glowR);
            glowGrad.setColorAt(0.0, QColor(190, 205, 245, 75));
            glowGrad.setColorAt(0.45, QColor(155, 170, 210, 30));
            glowGrad.setColorAt(1.0, QColor(0, 0, 0, 0));
            p.setPen(Qt::NoPen);
            p.setBrush(glowGrad);
            p.drawEllipse(QPointF(sx, sy), glowR, glowR);

            // Moon disk with a subtle phase-like gradient.
            QRadialGradient moonGrad(QPointF(sx - r * 0.25, sy - r * 0.25), r * 1.35);
            moonGrad.setColorAt(0.0, QColor(252, 252, 245));
            moonGrad.setColorAt(0.62, QColor(220, 224, 230));
            moonGrad.setColorAt(1.0, QColor(130, 136, 148));
            p.setBrush(moonGrad);
            p.drawEllipse(QPointF(sx, sy), r, r);

            // Phase mask: the dark limb is oriented opposite to the Sun direction.
            double dirX = -1.0;
            double dirY = 0.0;
            if (hasSunPix) {
                const double vx = sunPix.x() - sx;
                const double vy = sunPix.y() - sy;
                const double vn = std::hypot(vx, vy);
                if (vn > 1e-9) {
                    dirX = vx / vn;
                    dirY = vy / vn;
                }
            }
            const double terminator = 1.0 - 2.0 * illumination;
            const double shadowShift = terminator * r;

            p.save();
            QPainterPath moonDiskClip;
            moonDiskClip.addEllipse(QPointF(sx, sy), r, r);
            p.setClipPath(moonDiskClip);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(12, 15, 22, 165));
            p.drawEllipse(
                QPointF(sx + dirX * shadowShift, sy + dirY * shadowShift),
                r,
                r
            );
            p.restore();

            m_pixelRadii[i] = r;
        } else if (id == VENUS_ID) {
            const double bf = 27.0 / std::pow(2.512, m);
            const double r = std::max(5.0, std::clamp(1.5 * std::sqrt(std::max(0.0, bf)), 1.0, 4.0) * starSizeFactor);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(255, 245, 170));
            p.drawEllipse(QPointF(sx, sy), r, r);
            m_pixelRadii[i] = r;
        } else if (id == MARS_ID) {
            const double bf = 27.0 / std::pow(2.512, m);
            const double r = std::max(4.0, std::clamp(1.5 * std::sqrt(std::max(0.0, bf)), 1.0, 4.0) * starSizeFactor);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(255, 150, 120));
            p.drawEllipse(QPointF(sx, sy), r, r);
            m_pixelRadii[i] = r;
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
            m_pixelRadii[i] = r;
        }
    }
}

void StarMapWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.drawImage(rect(), blurredImage);

    if (m_selectedIndex >= 0 &&
        m_selectedIndex < static_cast<int>(m_pixelPositions.size()) &&
        m_selectedIndex < static_cast<int>(m_pixelRadii.size()))
    {
        const QPointF& pos = m_pixelPositions[m_selectedIndex];
        double r = m_pixelRadii[m_selectedIndex];
        double margin = 3.0;
        QPen pen(QColor(255, 60, 60));
        pen.setStyle(Qt::DashLine);
        pen.setWidth(2);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(pos, r + margin, r + margin);
    }
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
        m_selectedIndex = foundIndex;
        update();
    } else {
        hideInfoPopup();
        m_selectedIndex = -1;
    }
} else if (event->button() == Qt::LeftButton) {
    hideInfoPopup();
    m_selectedIndex = -1;
}

QWidget::mousePressEvent(event);
}

void StarMapWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_S || event->key() == Qt::Key_7) {
        qDebug() << "[StarMapWidget] keyPress detected" << event->text();
        if (saveSnapshot()) {
            qDebug() << "[StarMapWidget] snapshot saved";
            event->accept();
            return;
        } else {
            qDebug() << "[StarMapWidget] snapshot FAILED";
        }
    }
    QWidget::keyPressEvent(event);
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

bool StarMapWidget::saveSnapshot()
{
    QString saveDirPath = ensureSaveDirectory();
    if (saveDirPath.isEmpty())
        return false;

    QDir dir(saveDirPath);
    int index = nextSnapshotIndex(dir);
    QString baseName = QString("sky_%1").arg(index);

    QString imagePath = dir.filePath(baseName + ".png");
    if (!blurredImage.save(imagePath)) {
        qWarning() << "[StarMapWidget] failed to save image" << imagePath;
        return false;
    }

    QFile txtFile(dir.filePath(baseName + ".txt"));
    if (!txtFile.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    auto radToDeg = [](double rad) {
        return rad * 180.0 / M_PI;
    };

    QTextStream stream(&txtFile);
    stream.setRealNumberNotation(QTextStream::FixedNotation);
    stream.setRealNumberPrecision(8);
    stream << "STAR_ID\tRA(rad)\tDEC(rad)\n";
    for (const auto& proj : m_projections) {
        stream << proj.starId << '\t'
               << proj.raRad << '\t'
               << proj.decRad << '\n';
    }

    stream << "\n";
    stream.setRealNumberPrecision(4);
    stream << "Observer RA (deg): " << radToDeg(m_observation.observerRaRad) << '\n';
    stream << "Observer Dec (deg): " << radToDeg(m_observation.observerDecRad) << '\n';
    stream << "FOV X (deg): " << radToDeg(m_observation.fovXRad) << '\n';
    stream << "FOV Y (deg): " << radToDeg(m_observation.fovYRad) << '\n';
    stream << "Observation date: "
           << QString("%1-%2-%3")
                .arg(m_observation.obsYear, 4, 10, QLatin1Char('0'))
                .arg(m_observation.obsMonth, 2, 10, QLatin1Char('0'))
                .arg(m_observation.obsDay, 2, 10, QLatin1Char('0'))
           << '\n';

    qDebug() << "[StarMapWidget] snapshot stored at" << imagePath;
    return true;
}

QString StarMapWidget::ensureSaveDirectory() const
{
    const QString relative = QStringLiteral("save");
    QStringList roots = {
        QCoreApplication::applicationDirPath(),
        QDir::currentPath()
    };

    for (const QString& root : roots) {
        QDir dir(root);
        for (int depth = 0; depth < 6; ++depth) {
            const QString candidate = dir.absoluteFilePath(relative);
            QDir candidateDir(candidate);
            if (candidateDir.exists())
                return candidateDir.absolutePath();
            if (QDir().mkpath(candidate))
                return candidateDir.absolutePath();
            if (!dir.cdUp())
                break;
        }
    }
    const QString fallback =
        QDir::cleanPath(QStringLiteral("/Users/lehacho/starry_sky/") + "/" + relative);
    QDir fallbackDir(fallback);
    if (fallbackDir.exists() || QDir().mkpath(fallback))
        return fallbackDir.absolutePath();
    qWarning() << "[StarMapWidget] unable to create save directory";
    return {};
}

int StarMapWidget::nextSnapshotIndex(const QDir& dir) const
{
    QStringList files = dir.entryList(QStringList() << "sky_*.png", QDir::Files);
    int maxIndex = 0;
    for (const QString& file : files) {
        if (!file.startsWith("sky_") || !file.endsWith(".png"))
            continue;
        QString numberPart = file.mid(4, file.length() - 8);
        bool ok = false;
        int value = numberPart.toInt(&ok);
        if (ok && value > maxIndex)
            maxIndex = value;
    }
    return maxIndex + 1;
}
