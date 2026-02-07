#pragma once

#include <cmath>

namespace astro {

struct Observer {
    double lat_rad;   // геодезическая широта, рад
    double lon_rad;   // геодезическая долгота (восток +), рад
    double height_m;  // высота над уровнем моря, м
};

struct SunEquatorial {
    double ra;          // прямое восхождение, рад (0..2π)
    double dec;         // склонение, рад (-π/2..π/2)
    double distance_au; // расстояние до Солнца, а.е.
};

inline double centuries_TT(double jd_tt)
{
    return (jd_tt - 2451545.0) / 36525.0;
}

inline double norm_deg(double x)
{
    double r = std::fmod(x, 360.0);
    return (r < 0.0) ? r + 360.0 : r;
}

constexpr double DEG2RAD = M_PI / 180.0;
constexpr double ARCSEC2RAD = DEG2RAD / 3600.0;

double mean_obliquity_rad(double T);

SunEquatorial sun_apparent_geocentric(double jd_tt);

SunEquatorial sun_topocentric(double jd_tt, const Observer& obs);

} // namespace astro

