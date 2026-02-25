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
#include <array>
#include <unordered_map>

static constexpr uint64_t SUN_ID = astro::bodyIdValue(astro::BodyId::Sun);
static constexpr uint64_t MOON_ID = astro::bodyIdValue(astro::BodyId::Moon);
static constexpr uint64_t MERCURY_ID = astro::bodyIdValue(astro::BodyId::Mercury);
static constexpr uint64_t VENUS_ID = astro::bodyIdValue(astro::BodyId::Venus);
static constexpr uint64_t MARS_ID = astro::bodyIdValue(astro::BodyId::Mars);
static constexpr uint64_t JUPITER_ID = astro::bodyIdValue(astro::BodyId::Jupiter);
static constexpr uint64_t SATURN_ID = astro::bodyIdValue(astro::BodyId::Saturn);
static constexpr uint64_t URANUS_ID = astro::bodyIdValue(astro::BodyId::Uranus);
static constexpr uint64_t NEPTUNE_ID = astro::bodyIdValue(astro::BodyId::Neptune);

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

double clamp01(double x)
{
    return std::clamp(x, 0.0, 1.0);
}

double smoothstep(double a, double b, double x)
{
    if (b <= a)
        return x < a ? 0.0 : 1.0;
    double t = clamp01((x - a) / (b - a));
    return t * t * (3.0 - 2.0 * t);
}

double lerp(double a, double b, double t)
{
    return a + (b - a) * t;
}

double pixelDiameterFromAngular(double angularDiameterRad, double scale)
{
    if (!(angularDiameterRad > 0.0) || !(scale > 0.0))
        return 0.0;
    return 2.0 * scale * std::tan(0.5 * angularDiameterRad);
}

std::array<double, 3> normalizeVec(const std::array<double, 3>& v)
{
    const double n = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (n <= 1e-15)
        return {0.0, 0.0, 1.0};
    return {v[0] / n, v[1] / n, v[2] / n};
}

double noiseHash(int x, int y)
{
    uint32_t n = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(y) * 668265263u;
    n = (n ^ (n >> 13u)) * 1274126177u;
    n ^= (n >> 16u);
    return (n & 0x00FFFFFFu) / static_cast<double>(0x00FFFFFFu);
}

double valueNoise(double x, double y)
{
    const int xi = static_cast<int>(std::floor(x));
    const int yi = static_cast<int>(std::floor(y));
    const double tx = x - xi;
    const double ty = y - yi;

    const double v00 = noiseHash(xi, yi);
    const double v10 = noiseHash(xi + 1, yi);
    const double v01 = noiseHash(xi, yi + 1);
    const double v11 = noiseHash(xi + 1, yi + 1);

    const double sx = smoothstep(0.0, 1.0, tx);
    const double sy = smoothstep(0.0, 1.0, ty);

    const double a = v00 + (v10 - v00) * sx;
    const double b = v01 + (v11 - v01) * sx;
    return a + (b - a) * sy;
}

double fbm(double x, double y, int octaves = 4)
{
    double sum = 0.0;
    double amp = 0.5;
    double freq = 1.0;
    for (int i = 0; i < octaves; ++i) {
        sum += amp * valueNoise(x * freq, y * freq);
        freq *= 2.0;
        amp *= 0.5;
    }
    return clamp01(sum);
}

QColor starColorFromBv(double bv, int brightness)
{
    bv = std::clamp(bv, -0.4, 2.0);
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;

    if (bv < 0.0) {
        const double t = (bv + 0.4) / 0.4;
        r = 0.61 + 0.11 * t + 0.10 * t * t;
    } else if (bv < 0.4) {
        const double t = bv / 0.4;
        r = 0.83 + 0.17 * t;
    } else {
        r = 1.0;
    }

    if (bv < 0.0) {
        const double t = (bv + 0.4) / 0.4;
        g = 0.70 + 0.07 * t + 0.10 * t * t;
    } else if (bv < 0.4) {
        const double t = bv / 0.4;
        g = 0.87 + 0.11 * t;
    } else if (bv < 1.6) {
        const double t = (bv - 0.4) / 1.2;
        g = 0.98 - 0.16 * t;
    } else {
        const double t = (bv - 1.6) / 0.4;
        g = 0.82 - 0.5 * t * t;
    }

    if (bv < 0.4) {
        b = 1.0;
    } else if (bv < 1.5) {
        const double t = (bv - 0.4) / 1.1;
        b = 1.0 - 0.47 * t + 0.10 * t * t;
    } else if (bv < 1.94) {
        const double t = (bv - 1.5) / 0.44;
        b = 0.63 - 0.6 * t * t;
    } else {
        b = 0.0;
    }

    const double scale = std::clamp(brightness / 255.0, 0.0, 1.0);
    const int rr = std::clamp(static_cast<int>(std::lround(255.0 * r * scale)), 0, 255);
    const int gg = std::clamp(static_cast<int>(std::lround(255.0 * g * scale)), 0, 255);
    const int bb = std::clamp(static_cast<int>(std::lround(255.0 * b * scale)), 0, 255);
    return QColor(rr, gg, bb);
}

struct BodySpriteKey {
    uint64_t bodyId = 0;
    int radiusPx = 0;
    int phaseBin = 0;

    bool operator==(const BodySpriteKey& other) const
    {
        return bodyId == other.bodyId
            && radiusPx == other.radiusPx
            && phaseBin == other.phaseBin;
    }
};

struct BodySpriteKeyHash {
    std::size_t operator()(const BodySpriteKey& k) const
    {
        std::size_t h1 = std::hash<uint64_t>{}(k.bodyId);
        std::size_t h2 = std::hash<int>{}(k.radiusPx);
        std::size_t h3 = std::hash<int>{}(k.phaseBin);
        return h1 ^ (h2 << 1) ^ (h3 << 7);
    }
};

std::unordered_map<BodySpriteKey, QImage, BodySpriteKeyHash> g_bodySpriteCache;

QImage makeMoonSprite(int radiusPx, int phaseBin)
{
    const int r = std::max(1, radiusPx);
    const int pad = 2;
    const int size = 2 * r + 2 * pad;
    const double c = (size - 1) * 0.5;
    const double illum = std::clamp(phaseBin / 48.0, 0.0, 1.0);
    const double lz = 2.0 * illum - 1.0;
    const double lx = std::sqrt(std::max(0.0, 1.0 - lz * lz));

    QImage img(size, size, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    for (int y = 0; y < size; ++y) {
        auto* row = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < size; ++x) {
            const double nx = (x - c) / r;
            const double ny = (c - y) / r;
            const double rr = nx * nx + ny * ny;
            if (rr > 1.0) {
                row[x] = qRgba(0, 0, 0, 0);
                continue;
            }
            const double nz = std::sqrt(std::max(0.0, 1.0 - rr));
            const double ndotl = nx * lx + nz * lz;
            const double lambert = std::max(0.0, ndotl);

            const double n1 = fbm(nx * 3.5 + 17.3, ny * 3.5 - 5.1, 4);
            const double n2 = fbm(nx * 8.1 - 9.4, ny * 8.1 + 3.2, 3);
            const double mare = smoothstep(0.50, 0.75, n1 * 0.75 + n2 * 0.25);
            const double albedo = std::clamp(0.62 + 0.24 * n1 - 0.26 * mare, 0.28, 0.92);

            const double shade = 0.10 + 0.90 * lambert;
            const double rim = std::pow(1.0 - nz, 3.0) * 0.18;
            double rCol = albedo * 0.98 * shade + rim * 0.25;
            double gCol = albedo * 0.97 * shade + rim * 0.24;
            double bCol = albedo * 0.95 * shade + rim * 0.23;

            const int alpha = static_cast<int>(std::lround(255.0 * smoothstep(1.0, 0.94, rr)));
            row[x] = qRgba(
                std::clamp(static_cast<int>(std::lround(rCol * 255.0)), 0, 255),
                std::clamp(static_cast<int>(std::lround(gCol * 255.0)), 0, 255),
                std::clamp(static_cast<int>(std::lround(bCol * 255.0)), 0, 255),
                alpha
            );
        }
    }
    return img;
}

QImage makeVenusSprite(int radiusPx, int phaseBin)
{
    const int r = std::max(1, radiusPx);
    const int pad = 2;
    const int size = 2 * r + 2 * pad;
    const double c = (size - 1) * 0.5;
    const double illum = std::clamp(phaseBin / 48.0, 0.0, 1.0);
    const double lz = 2.0 * illum - 1.0;
    const double lx = std::sqrt(std::max(0.0, 1.0 - lz * lz));

    QImage img(size, size, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    for (int y = 0; y < size; ++y) {
        auto* row = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < size; ++x) {
            const double nx = (x - c) / r;
            const double ny = (c - y) / r;
            const double rr = nx * nx + ny * ny;
            if (rr > 1.0) {
                row[x] = qRgba(0, 0, 0, 0);
                continue;
            }
            const double nz = std::sqrt(std::max(0.0, 1.0 - rr));
            const double ndotl = nx * lx + nz * lz;
            const double lambert = std::max(0.0, ndotl);
            const double softLight = std::pow(lambert, 0.55);
            const double cloud = 0.92 + 0.08 * fbm(nx * 2.8 + 4.0, ny * 2.8 - 11.0, 3);
            const double limb = std::pow(1.0 - nz, 2.6) * (0.22 + 0.25 * (1.0 - illum));

            double rCol = (0.96 * cloud) * (0.20 + 0.80 * softLight) + limb * 0.28;
            double gCol = (0.93 * cloud) * (0.20 + 0.80 * softLight) + limb * 0.24;
            double bCol = (0.84 * cloud) * (0.20 + 0.80 * softLight) + limb * 0.16;

            const int alpha = static_cast<int>(std::lround(255.0 * smoothstep(1.0, 0.94, rr)));
            row[x] = qRgba(
                std::clamp(static_cast<int>(std::lround(rCol * 255.0)), 0, 255),
                std::clamp(static_cast<int>(std::lround(gCol * 255.0)), 0, 255),
                std::clamp(static_cast<int>(std::lround(bCol * 255.0)), 0, 255),
                alpha
            );
        }
    }
    return img;
}

QImage makeMarsSprite(int radiusPx, int phaseBin)
{
    const int r = std::max(1, radiusPx);
    const int pad = 2;
    const int size = 2 * r + 2 * pad;
    const double c = (size - 1) * 0.5;
    const double illum = std::clamp(phaseBin / 48.0, 0.0, 1.0);
    const double lz = 2.0 * illum - 1.0;
    const double lx = std::sqrt(std::max(0.0, 1.0 - lz * lz));

    QImage img(size, size, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    for (int y = 0; y < size; ++y) {
        auto* row = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < size; ++x) {
            const double nx = (x - c) / r;
            const double ny = (c - y) / r;
            const double rr = nx * nx + ny * ny;
            if (rr > 1.0) {
                row[x] = qRgba(0, 0, 0, 0);
                continue;
            }
            const double nz = std::sqrt(std::max(0.0, 1.0 - rr));
            const double ndotl = nx * lx + nz * lz;
            const double lambert = std::max(0.0, ndotl);
            const double terrain = fbm(nx * 3.2 + 1.7, ny * 3.2 - 7.9, 4);
            const double darkSpot = std::exp(-((nx - 0.20) * (nx - 0.20) + (ny + 0.08) * (ny + 0.08)) / 0.05);
            const double cap = smoothstep(0.60, 0.86, ny + 0.08 * terrain);

            double dust = std::clamp(0.62 + 0.26 * terrain - 0.30 * darkSpot, 0.20, 1.0);
            const double shade = 0.16 + 0.84 * lambert;
            const double haze = std::pow(1.0 - nz, 2.0) * 0.15;

            double rCol = dust * 0.86 * shade + haze * 0.10 + cap * 0.22;
            double gCol = dust * 0.50 * shade + haze * 0.13 + cap * 0.24;
            double bCol = dust * 0.34 * shade + haze * 0.20 + cap * 0.28;

            const int alpha = static_cast<int>(std::lround(255.0 * smoothstep(1.0, 0.94, rr)));
            row[x] = qRgba(
                std::clamp(static_cast<int>(std::lround(rCol * 255.0)), 0, 255),
                std::clamp(static_cast<int>(std::lround(gCol * 255.0)), 0, 255),
                std::clamp(static_cast<int>(std::lround(bCol * 255.0)), 0, 255),
                alpha
            );
        }
    }
    return img;
}

QImage bodySpriteCached(uint64_t bodyId, int radiusPx, double illumination)
{
    const int phaseBin = std::clamp(static_cast<int>(std::lround(clamp01(illumination) * 48.0)), 0, 48);
    const BodySpriteKey key{bodyId, std::max(1, radiusPx), phaseBin};
    auto it = g_bodySpriteCache.find(key);
    if (it != g_bodySpriteCache.end())
        return it->second;

    QImage generated;
    if (bodyId == MOON_ID) {
        generated = makeMoonSprite(key.radiusPx, key.phaseBin);
    } else if (bodyId == VENUS_ID) {
        generated = makeVenusSprite(key.radiusPx, key.phaseBin);
    } else if (bodyId == MARS_ID) {
        generated = makeMarsSprite(key.radiusPx, key.phaseBin);
    } else {
        generated = QImage();
    }
    g_bodySpriteCache.emplace(key, generated);
    return generated;
}
} // namespace

StarMapWidget::StarMapWidget(
    std::vector<StarProjection> projections,
    const StarCatalog::Sun&      sunInfo,
    const BlurParams             blurParams,
    bool                         blurEnabled,
    const FlareParams            flareParams,
    bool                         flareEnabled,
    PlanetRenderSizeMode         planetSizeMode,
    ObservationInfo              observation,
    bool                         zoomActive,
    double                       zoomFactor,
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
      m_planetSizeMode(planetSizeMode),
      m_infoPopup(nullptr),
      m_infoLabel(nullptr),
      m_observation(observation),
      m_zoomActive(zoomActive),
      m_zoomFactor(zoomFactor)
{
    setFocusPolicy(Qt::StrongFocus);
    // 1) Цветной холст (сохраняем всю текущую логику рендера, blur и flare)
    starMapImage = QImage(1081, 761, QImage::Format_ARGB32_Premultiplied);
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

    const double limitX = std::tan(std::max(1e-7, m_observation.fovXRad));
    const double limitY = std::tan(std::max(1e-7, m_observation.fovYRad));
    if (!(std::isfinite(limitX) && std::isfinite(limitY)) || limitX <= 0.0 || limitY <= 0.0) {
        m_pixelPositions.clear();
        m_hasGeometry = false;
        return;
    }

    m_minXi = -limitX;
    m_maxXi = limitX;
    m_minEta = -limitY;
    m_maxEta = limitY;
    m_centerXi = 0.0;
    m_centerEta = 0.0;
    m_scale = std::min((W * 0.5) / limitX, (H * 0.5) / limitY);
    m_hasGeometry = true;

    const double starSizeFactor = 2.0;
    const double sunVisualRadiusBasePx = 53.0;
    QPointF sunPix;
    bool hasSunPix = false;
    double sunXi = 0.0;
    double sunEta = 0.0;
    bool hasSunDir = false;
    double sunAngularDiameterRad = 0.0;
    bool hasSunAngularDiameter = false;

    for (const auto& proj : m_projections) {
        const uint64_t id = static_cast<uint64_t>(proj.starId);
        if (id != SUN_ID)
            continue;
        sunXi = proj.x;
        sunEta = proj.y;
        hasSunDir = true;
        if (proj.angularDiameterRad > 0.0) {
            sunAngularDiameterRad = proj.angularDiameterRad;
            hasSunAngularDiameter = true;
        }
        const double sx = W / 2.0 + (proj.x - m_centerXi) * m_scale;
        const double sy = H / 2.0 - (proj.y - m_centerEta) * m_scale;
        sunPix = QPointF(sx, sy);
        hasSunPix = true;
        break;
    }

    m_pixelPositions.resize(m_projections.size());
    m_pixelRadii.resize(m_projections.size());

    const double modeBlendBase = (m_planetSizeMode == PlanetRenderSizeMode::Enhanced) ? 1.0 : 0.0;
    const double zoomBlend = m_zoomActive ? smoothstep(1.05, 6.0, std::max(1.0, m_zoomFactor)) : 0.0;
    const double sizeBlend = std::max(modeBlendBase, zoomBlend);
    const double enhancedZoomGain = m_zoomActive ? std::pow(std::max(1.0, m_zoomFactor), 0.18) : 1.0;
    const double sunZoomGain = m_zoomActive ? std::pow(std::max(1.0, m_zoomFactor), 0.22) : 1.0;
    const double sunScale = lerp(1.0, sunZoomGain, zoomBlend);
    const double sunRenderRadiusPx = sunVisualRadiusBasePx * sunScale;

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
            // Солнце: нейтрально-белый стиль без чрезмерной желтизны.
            QRadialGradient sunGrad(QPointF(sx - 9.0 * sunScale, sy - 10.0 * sunScale), sunRenderRadiusPx * 1.25);
            sunGrad.setColorAt(0.0, QColor(255, 255, 252, 255));
            sunGrad.setColorAt(0.58, QColor(246, 244, 236, 245));
            sunGrad.setColorAt(1.0, QColor(232, 226, 205, 210));
            p.setPen(Qt::NoPen);
            p.setBrush(sunGrad);
            p.drawEllipse(QPointF(sx, sy), sunRenderRadiusPx * 0.93, sunRenderRadiusPx);
            m_pixelRadii[i] = sunRenderRadiusPx;
        } else if (id == MOON_ID) {
            const double moonAngularDiamRad = (proj.angularDiameterRad > 0.0)
                ? proj.angularDiameterRad
                : (0.52 * M_PI / 180.0);

            const double moonDiamReal = pixelDiameterFromAngular(moonAngularDiamRad, m_scale);
            double moonDiamStyled = moonDiamReal;
            if (hasSunAngularDiameter && sunAngularDiameterRad > 1e-10) {
                const double ratioToSun = std::clamp(moonAngularDiamRad / sunAngularDiameterRad, 0.75, 1.25);
                moonDiamStyled = (2.0 * sunRenderRadiusPx) * ratioToSun * 0.90;
            } else {
                moonDiamStyled = moonDiamReal * 1.8;
            }
            moonDiamStyled *= enhancedZoomGain;

            const double moonBlend = std::max(sizeBlend, 0.35);
            double moonDiamPx = lerp(moonDiamReal, moonDiamStyled, moonBlend);
            moonDiamPx = std::max(1.8, moonDiamPx);
            const double r = 0.5 * moonDiamPx;
            const double illumination = std::clamp(proj.illumination, 0.0, 1.0);

            double angleDeg = 0.0;
            bool angleResolved = false;
            if (hasSunDir) {
                const std::array<double, 3> moonDirCam = normalizeVec({proj.x, proj.y, 1.0});
                const std::array<double, 3> sunDirCam = normalizeVec({sunXi, sunEta, 1.0});
                const double sm = sunDirCam[0] * moonDirCam[0]
                                + sunDirCam[1] * moonDirCam[1]
                                + sunDirCam[2] * moonDirCam[2];
                std::array<double, 3> pTerm = {
                    sunDirCam[0] - sm * moonDirCam[0],
                    sunDirCam[1] - sm * moonDirCam[1],
                    sunDirCam[2] - sm * moonDirCam[2]
                };
                const double pNorm = std::sqrt(pTerm[0] * pTerm[0] + pTerm[1] * pTerm[1] + pTerm[2] * pTerm[2]);
                if (pNorm > 1e-12) {
                    pTerm[0] /= pNorm;
                    pTerm[1] /= pNorm;
                    // pTerm[2] ignored in screen-angle mapping.
                    angleDeg = std::atan2(-pTerm[1], pTerm[0]) * 180.0 / M_PI;
                    angleResolved = true;
                }
            }
            if (!angleResolved && hasSunPix) {
                const double vx = sunPix.x() - sx;
                const double vy = sunPix.y() - sy;
                const double vn = std::hypot(vx, vy);
                if (vn > 1e-9)
                    angleDeg = std::atan2(vy / vn, vx / vn) * 180.0 / M_PI;
            }

            // Soft moon glow (intentionally reduced vs previous version)
            const double glowR = r * 1.22;
            QRadialGradient glowGrad(QPointF(sx, sy), glowR);
            glowGrad.setColorAt(0.0, QColor(205, 214, 245, 22));
            glowGrad.setColorAt(0.45, QColor(165, 177, 214, 8));
            glowGrad.setColorAt(1.0, QColor(0, 0, 0, 0));
            p.setPen(Qt::NoPen);
            p.setBrush(glowGrad);
            p.drawEllipse(QPointF(sx, sy), glowR, glowR);

            QImage moonSprite = bodySpriteCached(MOON_ID, static_cast<int>(std::lround(r)), illumination);
            p.save();
            p.translate(sx, sy);
            p.rotate(angleDeg);
            p.drawImage(QPointF(-moonSprite.width() * 0.5, -moonSprite.height() * 0.5), moonSprite);
            p.restore();

            m_pixelRadii[i] = r;
        } else if (id == VENUS_ID) {
            const double bf = 27.0 / std::pow(2.512, m);
            const double diamPhysPx = pixelDiameterFromAngular(proj.angularDiameterRad, m_scale);
            double rReal = 0.5 * diamPhysPx;
            if (!(rReal > 0.0))
                rReal = std::clamp(0.35 * std::sqrt(std::max(0.0, bf)), 0.35, 1.2);
            rReal = std::max(0.70, rReal);

            double rEnhanced = std::max(5.0, std::clamp(1.5 * std::sqrt(std::max(0.0, bf)), 1.0, 4.0) * starSizeFactor);
            rEnhanced *= enhancedZoomGain;
            const double r = lerp(rReal, rEnhanced, sizeBlend);
            const double illumination = std::clamp(proj.illumination, 0.0, 1.0);
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
            const double angleDeg = std::atan2(dirY, dirX) * 180.0 / M_PI;

            const double glowReal = std::max(r * 2.4, 2.2 + std::max(0.0, -m) * 0.45);
            const double glowEnhanced = r * 2.5;
            const double glowRadius = lerp(glowReal, glowEnhanced, sizeBlend);
            QRadialGradient glowGrad(QPointF(sx, sy), glowRadius);
            glowGrad.setColorAt(0.0, QColor(255, 244, 200, 72));
            glowGrad.setColorAt(1.0, QColor(0, 0, 0, 0));
            p.setPen(Qt::NoPen);
            p.setBrush(glowGrad);
            p.drawEllipse(QPointF(sx, sy), glowRadius, glowRadius);

            if (sizeBlend > 0.22 || r >= 1.6) {
                QImage venusSprite = bodySpriteCached(VENUS_ID, static_cast<int>(std::lround(r)), illumination);
                p.save();
                p.translate(sx, sy);
                p.rotate(angleDeg);
                p.drawImage(QPointF(-venusSprite.width() * 0.5, -venusSprite.height() * 0.5), venusSprite);
                p.restore();
            } else {
                p.setBrush(QColor(244, 235, 206));
                p.setPen(Qt::NoPen);
                p.drawEllipse(QPointF(sx, sy), r, r);
            }
            m_pixelRadii[i] = r;
        } else if (id == MARS_ID) {
            const double bf = 27.0 / std::pow(2.512, m);
            const double diamPhysPx = pixelDiameterFromAngular(proj.angularDiameterRad, m_scale);
            double rReal = 0.5 * diamPhysPx;
            if (!(rReal > 0.0))
                rReal = std::clamp(0.32 * std::sqrt(std::max(0.0, bf)), 0.30, 1.0);
            rReal = std::max(0.60, rReal);

            double rEnhanced = std::max(4.0, std::clamp(1.5 * std::sqrt(std::max(0.0, bf)), 1.0, 4.0) * starSizeFactor);
            rEnhanced *= enhancedZoomGain;
            const double r = lerp(rReal, rEnhanced, sizeBlend);
            const double illumination = std::clamp(proj.illumination, 0.0, 1.0);
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
            const double angleDeg = std::atan2(dirY, dirX) * 180.0 / M_PI;

            const double hazeReal = std::max(r * 1.8, 1.8 + std::max(0.0, -m) * 0.22);
            const double hazeEnhanced = r * 1.7;
            const double hazeR = lerp(hazeReal, hazeEnhanced, sizeBlend);
            QRadialGradient haze(QPointF(sx, sy), hazeR);
            haze.setColorAt(0.0, QColor(180, 210, 245, 16));
            haze.setColorAt(1.0, QColor(0, 0, 0, 0));
            p.setPen(Qt::NoPen);
            p.setBrush(haze);
            p.drawEllipse(QPointF(sx, sy), hazeR, hazeR);

            if (sizeBlend > 0.22 || r >= 1.6) {
                QImage marsSprite = bodySpriteCached(MARS_ID, static_cast<int>(std::lround(r)), illumination);
                p.save();
                p.translate(sx, sy);
                p.rotate(angleDeg);
                p.drawImage(QPointF(-marsSprite.width() * 0.5, -marsSprite.height() * 0.5), marsSprite);
                p.restore();
            } else {
                p.setBrush(QColor(201, 120, 88));
                p.setPen(Qt::NoPen);
                p.drawEllipse(QPointF(sx, sy), r, r);
            }
            m_pixelRadii[i] = r;
        } else if (id == MERCURY_ID || id == JUPITER_ID || id == SATURN_ID
                   || id == URANUS_ID || id == NEPTUNE_ID) {
            const double bf = 27.0 / std::pow(2.512, m);
            const double diamPhysPx = pixelDiameterFromAngular(proj.angularDiameterRad, m_scale);
            double rReal = 0.5 * diamPhysPx;
            if (!(rReal > 0.0))
                rReal = std::clamp(0.30 * std::sqrt(std::max(0.0, bf)), 0.28, 0.9);
            rReal = std::max(0.55, rReal);

            const double baseEnhanced =
                std::clamp(1.35 * std::sqrt(std::max(0.0, bf)), 1.0, 4.0) * starSizeFactor;
            double rEnhanced = 1.0;
            if (id == MERCURY_ID) {
                rEnhanced = std::max(3.1, baseEnhanced * 0.95);
            } else if (id == JUPITER_ID) {
                rEnhanced = std::max(6.0, baseEnhanced * 1.30);
            } else if (id == SATURN_ID) {
                rEnhanced = std::max(5.2, baseEnhanced * 1.15);
            } else if (id == URANUS_ID) {
                rEnhanced = std::max(4.2, baseEnhanced * 1.00);
            } else if (id == NEPTUNE_ID) {
                rEnhanced = std::max(4.0, baseEnhanced * 0.95);
            }
            rEnhanced *= enhancedZoomGain;
            const double r = lerp(rReal, rEnhanced, sizeBlend);

            QColor baseColor(220, 220, 220);
            int glowAlpha = 18;
            if (id == MERCURY_ID) {
                baseColor = QColor(192, 186, 176);   // warm gray
                glowAlpha = 12;
            } else if (id == JUPITER_ID) {
                baseColor = QColor(214, 183, 138);   // beige-ochre
                glowAlpha = 26;
            } else if (id == SATURN_ID) {
                baseColor = QColor(224, 210, 160);   // pale yellow
                glowAlpha = 22;
            } else if (id == URANUS_ID) {
                baseColor = QColor(142, 212, 222);   // cyan-blue
                glowAlpha = 16;
            } else if (id == NEPTUNE_ID) {
                baseColor = QColor(98, 142, 232);    // deep blue
                glowAlpha = 14;
            }

            const double glowReal = std::max(r * 1.7, 1.6 + std::max(0.0, -m) * 0.18);
            const double glowEnhanced = r * 2.0;
            const double glowR = lerp(glowReal, glowEnhanced, sizeBlend);
            QRadialGradient glow(QPointF(sx, sy), glowR);
            glow.setColorAt(0.0, QColor(baseColor.red(), baseColor.green(), baseColor.blue(), glowAlpha));
            glow.setColorAt(1.0, QColor(0, 0, 0, 0));
            p.setPen(Qt::NoPen);
            p.setBrush(glow);
            p.drawEllipse(QPointF(sx, sy), glowR, glowR);

            QRadialGradient disk(QPointF(sx - 0.22 * r, sy - 0.24 * r), std::max(1.0, r * 1.35));
            disk.setColorAt(0.0, baseColor.lighter(128));
            disk.setColorAt(0.68, baseColor);
            disk.setColorAt(1.0, baseColor.darker(145));
            p.setBrush(disk);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(sx, sy), r, r);

            // A subtle Saturn ring hint in enhanced mode.
            if (id == SATURN_ID && sizeBlend > 0.20) {
                const int ringAlpha = static_cast<int>(std::lround(130.0 * std::clamp(sizeBlend, 0.0, 1.0)));
                QPen ringPen(QColor(220, 205, 150, std::clamp(ringAlpha, 30, 170)));
                ringPen.setWidthF(std::max(1.0, r * 0.22));
                p.setPen(ringPen);
                p.setBrush(Qt::NoBrush);
                p.drawEllipse(QPointF(sx, sy), r * 1.65, r * 0.72);
            }

            m_pixelRadii[i] = r;
        } else {
            // обычная звезда
            double bf = 27.0 / std::pow(2.512, m);
            int    br = std::clamp(int(bf*255.0), 40, 255);
            double baseR = std::clamp(1.5*std::sqrt(bf), 1.0, 4.0);
            double r     = baseR * starSizeFactor;

            QColor col = proj.hasColorIndex
                ? starColorFromBv(proj.colorIndex, br)
                : QColor(br, br, br);
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

    if (m_zoomActive) {
        p.setRenderHint(QPainter::Antialiasing, true);
        const QString zoomText = QString("ZOOM x%1").arg(m_zoomFactor, 0, 'f', 2);
        QFont f = p.font();
        f.setBold(true);
        f.setPointSize(std::max(10, f.pointSize() + 1));
        p.setFont(f);

        QFontMetrics fm(f);
        const int padX = 10;
        const int padY = 6;
        const int textW = fm.horizontalAdvance(zoomText);
        const int textH = fm.height();
        const QRect box(14, 14, textW + 2 * padX, textH + 2 * padY);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(20, 28, 45, 175));
        p.drawRoundedRect(box, 8.0, 8.0);
        p.setPen(QColor(220, 235, 255));
        p.drawText(box.adjusted(padX, padY, -padX, -padY), Qt::AlignLeft | Qt::AlignVCenter, zoomText);
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

void StarMapWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_hasGeometry && m_scale > 0.0) {
        const QPointF local = event->position();
        const double widgetW = std::max(1, width());
        const double widgetH = std::max(1, height());
        const double imageW = static_cast<double>(starMapImage.width());
        const double imageH = static_cast<double>(starMapImage.height());

        const double xImg = local.x() * imageW / widgetW;
        const double yImg = local.y() * imageH / widgetH;

        const double xi = (xImg - imageW * 0.5) / m_scale + m_centerXi;
        const double eta = (imageH * 0.5 - yImg) / m_scale + m_centerEta;
        emit zoomCenterRequested(xi, eta);
        event->accept();
        return;
    }

    QWidget::mouseDoubleClickEvent(event);
}

void StarMapWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Z) {
        emit zoomToggleRequested();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        emit zoomExitRequested();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) {
        emit zoomStepRequested(+1);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Minus || event->key() == Qt::Key_Underscore) {
        emit zoomStepRequested(-1);
        event->accept();
        return;
    }

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
