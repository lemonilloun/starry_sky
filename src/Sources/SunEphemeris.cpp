#include "SunEphemeris.h"
#include <algorithm>

namespace astro {

double mean_obliquity_rad(double T)
{
    double T2 = T * T;
    double T3 = T2 * T;
    double T4 = T3 * T;
    double T5 = T4 * T;
    double seconds = 84381.406
                   - 46.836769 * T
                   - 0.0001831 * T2
                   + 0.00200340 * T3
                   - 0.000000576 * T4
                   - 0.0000000434 * T5;
    return seconds * ARCSEC2RAD;
}

static void sincosd(double deg, double& s, double& c)
{
    double r = deg * DEG2RAD;
    s = std::sin(r);
    c = std::cos(r);
}

SunEquatorial sun_apparent_geocentric(double jd_tt)
{
    SunEquatorial out{};

    const double T = centuries_TT(jd_tt);
    const double T2 = T * T;

    double L0 = norm_deg(280.46646 + 36000.76983 * T + 0.0003032 * T2);
    double M  = norm_deg(357.52911 + 35999.05029 * T - 0.0001537 * T2);

    double e = 0.016708634 - 0.000042037 * T - 0.0000001267 * T2;

    double sM, cM;
    sincosd(M, sM, cM);
    double C = (1.914602 - 0.004817 * T - 0.000014 * T2) * sM
             + (0.019993 - 0.000101 * T) * std::sin(2.0 * DEG2RAD * M)
             + 0.000289 * std::sin(3.0 * DEG2RAD * M);

    double theta = L0 + C;

    double Omega = 125.04 - 1934.136 * T;

    double sOm, cOm;
    sincosd(Omega, sOm, cOm);
    double lambda = theta - 0.00569 - 0.00478 * sOm;

    double eps0 = mean_obliquity_rad(T);
    double eps  = eps0 + (0.00256 * cOm) * DEG2RAD;

    double v = M + C;
    double r = (1.000001018 * (1.0 - e * e)) / (1.0 + e * std::cos(v * DEG2RAD));

    double lam = lambda * DEG2RAD;
    double X = std::cos(lam);
    double Y = std::cos(eps) * std::sin(lam);
    double Z = std::sin(eps) * std::sin(lam);

    double ra = std::atan2(Y, X);
    if (ra < 0.0)
        ra += 2.0 * M_PI;
    double dec = std::asin(Z);

    out.ra = ra;
    out.dec = dec;
    out.distance_au = r;
    return out;
}

SunEquatorial sun_topocentric(double jd_tt, const Observer& obs)
{
    SunEquatorial g = sun_apparent_geocentric(jd_tt);

    double pi_rad = (8.794 * ARCSEC2RAD) / std::max(1e-9, g.distance_au);

    double sinPhi = std::sin(obs.lat_rad);
    double cosPhi = std::cos(obs.lat_rad);

    double cDec = std::cos(g.dec);
    double xs = cDec * std::cos(g.ra);
    double ys = cDec * std::sin(g.ra);
    double zs = std::sin(g.dec);

    double rho_cos_phi = cosPhi;
    double rho_sin_phi = sinPhi;

    double x_topo = xs - pi_rad * rho_cos_phi;
    double y_topo = ys;
    double z_topo = zs - pi_rad * rho_sin_phi;

    double r_topo = std::sqrt(x_topo * x_topo + y_topo * y_topo + z_topo * z_topo);
    x_topo /= r_topo;
    y_topo /= r_topo;
    z_topo /= r_topo;

    SunEquatorial out{};
    out.ra = std::atan2(y_topo, x_topo);
    if (out.ra < 0.0)
        out.ra += 2.0 * M_PI;
    out.dec = std::asin(z_topo);
    out.distance_au = g.distance_au;
    return out;
}

} // namespace astro

