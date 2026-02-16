#include "MoonEphemeris.h"

#include "SunEphemeris.h"

#include <algorithm>
#include <cmath>

namespace astro {
namespace {

constexpr double DEG_TO_RAD = M_PI / 180.0;
constexpr double KM_PER_AU = 149597870.7;
constexpr double MOON_RADIUS_KM = 1737.4;

inline double clampUnit(double x)
{
    return std::clamp(x, -1.0, 1.0);
}

struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

Vec3 operator-(const Vec3& a, const Vec3& b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 operator*(double k, const Vec3& a)
{
    return {k * a.x, k * a.y, k * a.z};
}

double dot(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

double norm(const Vec3& v)
{
    return std::sqrt(dot(v, v));
}

Vec3 normalize(const Vec3& v)
{
    const double n = norm(v);
    if (n <= 0.0)
        return {0.0, 0.0, 0.0};
    return {v.x / n, v.y / n, v.z / n};
}

double phaseAngleDeg(const Vec3& moonGeoEqAu, const Vec3& sunGeoEqAu)
{
    const Vec3 moonToSun = sunGeoEqAu - moonGeoEqAu;
    const Vec3 moonToEarth = -1.0 * moonGeoEqAu;
    const double c = clampUnit(dot(normalize(moonToSun), normalize(moonToEarth)));
    return std::acos(c) / DEG_TO_RAD;
}

} // namespace

BodyEquatorial MoonLiteProvider::computeMoon(double jd_tt) const
{
    const double T = centuries_TT(jd_tt);
    const double T2 = T * T;
    const double T3 = T2 * T;
    const double T4 = T3 * T;

    const double Lp = norm_deg(218.3164477
                               + 481267.88123421 * T
                               - 0.0015786 * T2
                               + T3 / 538841.0
                               - T4 / 65194000.0);

    const double D = norm_deg(297.8501921
                              + 445267.1114034 * T
                              - 0.0018819 * T2
                              + T3 / 545868.0
                              - T4 / 113065000.0);

    const double M = norm_deg(357.5291092
                              + 35999.0502909 * T
                              - 0.0001536 * T2
                              + T3 / 24490000.0);

    const double Mp = norm_deg(134.9633964
                               + 477198.8675055 * T
                               + 0.0087414 * T2
                               + T3 / 69699.0
                               - T4 / 14712000.0);

    const double F = norm_deg(93.2720950
                              + 483202.0175233 * T
                              - 0.0036539 * T2
                              - T3 / 3526000.0
                              + T4 / 863310000.0);

    auto sind = [](double deg) { return std::sin(deg * DEG_TO_RAD); };
    auto cosd = [](double deg) { return std::cos(deg * DEG_TO_RAD); };

    const double lambda = Lp
        + 6.289 * sind(Mp)
        + 1.274 * sind(2.0 * D - Mp)
        + 0.658 * sind(2.0 * D)
        + 0.214 * sind(2.0 * Mp)
        - 0.186 * sind(M)
        - 0.114 * sind(2.0 * F);

    const double beta =
          5.128 * sind(F)
        + 0.280 * sind(Mp + F)
        + 0.277 * sind(Mp - F)
        + 0.173 * sind(2.0 * D - F)
        + 0.055 * sind(2.0 * D + F - Mp)
        + 0.046 * sind(2.0 * D - F - Mp)
        + 0.033 * sind(2.0 * D + F)
        + 0.017 * sind(2.0 * Mp + F);

    const double distanceKm = 385000.56
        - 20905.0 * cosd(Mp)
        - 3699.0 * cosd(2.0 * D - Mp)
        - 2956.0 * cosd(2.0 * D)
        - 570.0 * cosd(2.0 * Mp)
        + 246.0 * cosd(2.0 * Mp - 2.0 * D)
        - 205.0 * cosd(M - 2.0 * D)
        - 171.0 * cosd(Mp + 2.0 * D)
        - 152.0 * cosd(Mp + M - 2.0 * D);

    const double Omega = norm_deg(125.04 - 1934.136 * T);
    const double eps = mean_obliquity_rad(T) + (0.00256 * cosd(Omega)) * DEG_TO_RAD;

    const double lam = lambda * DEG_TO_RAD;
    const double bet = beta * DEG_TO_RAD;

    const double cosBet = std::cos(bet);
    const Vec3 moonEcl {
        std::cos(lam) * cosBet,
        std::sin(lam) * cosBet,
        std::sin(bet)
    };

    const Vec3 moonEq {
        moonEcl.x,
        std::cos(eps) * moonEcl.y - std::sin(eps) * moonEcl.z,
        std::sin(eps) * moonEcl.y + std::cos(eps) * moonEcl.z
    };

    Vec3 moonEqAu = (distanceKm / KM_PER_AU) * moonEq;
    Vec3 moonEqNorm = normalize(moonEqAu);

    double ra = std::atan2(moonEqNorm.y, moonEqNorm.x);
    if (ra < 0.0)
        ra += 2.0 * M_PI;
    const double dec = std::asin(clampUnit(moonEqNorm.z));

    const SunEquatorial sun = sun_apparent_geocentric(jd_tt);
    const double cosSunDec = std::cos(sun.dec);
    const Vec3 sunEqAu {
        std::cos(sun.ra) * cosSunDec * sun.distance_au,
        std::sin(sun.ra) * cosSunDec * sun.distance_au,
        std::sin(sun.dec) * sun.distance_au
    };

    const double phaseDeg = phaseAngleDeg(moonEqAu, sunEqAu);
    const double magnitude = -12.73 + 0.026 * phaseDeg + 4.0e-9 * std::pow(phaseDeg, 4.0);

    BodyEquatorial out;
    out.raRad = ra;
    out.decRad = dec;
    out.distanceAu = distanceKm / KM_PER_AU;
    out.magnitude = magnitude;
    out.angularDiameterRad = 2.0 * std::atan(MOON_RADIUS_KM / std::max(1.0, distanceKm));
    out.bodyId = BodyId::Moon;
    out.name = "Moon";

    return out;
}

} // namespace astro
