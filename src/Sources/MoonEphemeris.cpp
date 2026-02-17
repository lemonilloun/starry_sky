#include "MoonEphemeris.h"

#include "SunEphemeris.h"

#include <algorithm>
#include <cmath>

namespace astro {
namespace {

constexpr double DEG_TO_RAD = M_PI / 180.0;
constexpr double ARCSEC_TO_RAD = DEG_TO_RAD / 3600.0;
constexpr double JULIAN_DAY_J2000 = 2451545.0;
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

struct NutationTerm {
    int D;
    int M;
    int Mp;
    int F;
    int Omega;
    double psiCoeff;    // 0.0001 arcsec
    double psiCoeffT;   // 0.0001 arcsec per century
    double epsCoeff;    // 0.0001 arcsec
    double epsCoeffT;   // 0.0001 arcsec per century
};

constexpr NutationTerm NUTATION_TERMS[] = {
    { 0,  0,  0,  0,  1, -171996.0, -174.2,  92025.0,   8.9 },
    { -2, 0,  0,  2,  2, -13187.0,   -1.6,   5736.0,  -3.1 },
    { 0,  0,  0,  2,  2, -2274.0,    -0.2,    977.0,  -0.5 },
    { 0,  0,  0,  0,  2,  2062.0,     0.2,   -895.0,   0.5 },
    { 0,  1,  0,  0,  0,  1426.0,    -3.4,     54.0,  -0.1 },
    { 0,  0,  1,  0,  0,   712.0,     0.1,     -7.0,   0.0 },
    { -2, 1,  0,  2,  2,  -517.0,     1.2,    224.0,  -0.6 },
    { 0,  0,  0,  2,  1,  -386.0,    -0.4,    200.0,   0.0 },
    { 0,  0,  1,  2,  2,  -301.0,     0.0,    129.0,  -0.1 },
    { -2, 0,  1,  2,  2,   217.0,    -0.5,    -95.0,   0.3 }
};

inline double normalizeDegrees(double deg)
{
    double res = std::fmod(deg, 360.0);
    if (res < 0.0)
        res += 360.0;
    return res;
}

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

void rotationX(double alpha, double R[3][3])
{
    const double c = std::cos(alpha);
    const double s = std::sin(alpha);
    R[0][0] = 1.0; R[0][1] = 0.0; R[0][2] = 0.0;
    R[1][0] = 0.0; R[1][1] = c;   R[1][2] = -s;
    R[2][0] = 0.0; R[2][1] = s;   R[2][2] = c;
}

void rotationY(double alpha, double R[3][3])
{
    const double c = std::cos(alpha);
    const double s = std::sin(alpha);
    R[0][0] = c;   R[0][1] = 0.0; R[0][2] = s;
    R[1][0] = 0.0; R[1][1] = 1.0; R[1][2] = 0.0;
    R[2][0] = -s;  R[2][1] = 0.0; R[2][2] = c;
}

void rotationZ(double alpha, double R[3][3])
{
    const double c = std::cos(alpha);
    const double s = std::sin(alpha);
    R[0][0] = c;   R[0][1] = -s;  R[0][2] = 0.0;
    R[1][0] = s;   R[1][1] = c;   R[1][2] = 0.0;
    R[2][0] = 0.0; R[2][1] = 0.0; R[2][2] = 1.0;
}

void mul3x3(const double A[3][3], const double B[3][3], double C[3][3])
{
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            double sum = 0.0;
            for (int k = 0; k < 3; ++k)
                sum += A[i][k] * B[k][j];
            C[i][j] = sum;
        }
    }
}

Vec3 mul3x3_3x1(const double M[3][3], const Vec3& v)
{
    return {
        M[0][0] * v.x + M[0][1] * v.y + M[0][2] * v.z,
        M[1][0] * v.x + M[1][1] * v.y + M[1][2] * v.z,
        M[2][0] * v.x + M[2][1] * v.y + M[2][2] * v.z
    };
}

void transpose3x3(const double A[3][3], double AT[3][3])
{
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            AT[j][i] = A[i][j];
}

double meanObliquity(double t)
{
    const double seconds = 84381.406
        - 46.836769 * t
        - 0.0001831 * t * t
        + 0.00200340 * t * t * t
        - 0.000000576 * t * t * t * t
        - 0.0000000434 * t * t * t * t * t;
    return seconds * ARCSEC_TO_RAD;
}

void buildPrecessionMatrix(double t, double P[3][3])
{
    const double t2 = t * t;
    const double t3 = t2 * t;
    const double t4 = t3 * t;
    const double t5 = t4 * t;

    const double ksi = 2.5976176
        + 2306.0809506 * t
        + 0.3019015 * t2
        + 0.0179663 * t3
        - 0.0000327 * t4
        - 0.0000002 * t5;

    const double tet = 2004.1917476 * t
        - 0.4269353 * t2
        - 0.0418251 * t3
        - 0.00006018 * t4
        - 0.0000001 * t5;

    const double z = -2.5976176
        + 2306.0803226 * t
        + 1.0947790 * t2
        + 0.0182273 * t3
        + 0.0000470 * t4
        - 0.0000003 * t5;

    const double rksi = (ksi / 3600.0) * DEG_TO_RAD;
    const double rtet = (tet / 3600.0) * DEG_TO_RAD;
    const double rz = (z / 3600.0) * DEG_TO_RAD;

    double R3ksi[3][3], R2tet[3][3], R3z[3][3], temp[3][3];
    rotationZ(-rksi, R3ksi);
    rotationY(rtet, R2tet);
    rotationZ(-rz, R3z);

    mul3x3(R2tet, R3z, temp);
    mul3x3(R3ksi, temp, P);
}

void computeNutationAngles(double t, double& deltaPsi, double& deltaEps)
{
    double D = normalizeDegrees(297.85036 + 445267.111480 * t - 0.0019142 * t * t + t * t * t / 189474.0);
    double M = normalizeDegrees(357.52772 + 35999.050340 * t - 0.0001603 * t * t - t * t * t / 300000.0);
    double Mp = normalizeDegrees(134.96298 + 477198.867398 * t + 0.0086972 * t * t + t * t * t / 56250.0);
    double F = normalizeDegrees(93.27191 + 483202.017538 * t - 0.0036825 * t * t + t * t * t / 327270.0);
    double Omega = normalizeDegrees(125.04452 - 1934.136261 * t + 0.0020708 * t * t + t * t * t / 450000.0);

    D *= DEG_TO_RAD;
    M *= DEG_TO_RAD;
    Mp *= DEG_TO_RAD;
    F *= DEG_TO_RAD;
    Omega *= DEG_TO_RAD;

    deltaPsi = 0.0;
    deltaEps = 0.0;

    for (const auto& term : NUTATION_TERMS) {
        const double arg = term.D * D + term.M * M + term.Mp * Mp + term.F * F + term.Omega * Omega;
        const double psiCoeff = term.psiCoeff + term.psiCoeffT * t;
        const double epsCoeff = term.epsCoeff + term.epsCoeffT * t;
        deltaPsi += psiCoeff * std::sin(arg);
        deltaEps += epsCoeff * std::cos(arg);
    }

    constexpr double scale = 1e-4 * ARCSEC_TO_RAD;
    deltaPsi *= scale;
    deltaEps *= scale;
}

void buildNutationMatrix(double epsMean, double deltaPsi, double deltaEps, double N[3][3])
{
    const double epsTrue = epsMean + deltaEps;

    double R1_neg_eps[3][3], R3_dpsi[3][3], R1_eps_true[3][3], temp[3][3];
    rotationX(-epsMean, R1_neg_eps);
    rotationZ(deltaPsi, R3_dpsi);
    rotationX(epsTrue, R1_eps_true);

    mul3x3(R3_dpsi, R1_eps_true, temp);
    mul3x3(R1_neg_eps, temp, N);
}

void buildPrecessionNutationMatrix(double jd, double PN[3][3])
{
    const double t = (jd - JULIAN_DAY_J2000) / 36525.0;

    double P[3][3];
    buildPrecessionMatrix(t, P);

    double dPsi = 0.0;
    double dEps = 0.0;
    computeNutationAngles(t, dPsi, dEps);

    const double epsMean = meanObliquity(t);
    double N[3][3];
    buildNutationMatrix(epsMean, dPsi, dEps, N);

    mul3x3(N, P, PN);
}

double phaseAngleDeg(const Vec3& moonGeoEqAu, const Vec3& sunGeoEqAu)
{
    const Vec3 moonToSun = sunGeoEqAu - moonGeoEqAu;
    const Vec3 moonToEarth = -1.0 * moonGeoEqAu;
    const double c = clampUnit(dot(normalize(moonToSun), normalize(moonToEarth)));
    return std::acos(c) / DEG_TO_RAD;
}

} // namespace

BodyEquatorial MoonLiteProvider::computeMoon(double jd_tt, const MoonObserverEqDate* observerEqDate) const
{
    // Low-order periodic lunar model (date-of-observation frame), then converted to J2000.
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
    const Vec3 moonEclDate {
        std::cos(lam) * cosBet,
        std::sin(lam) * cosBet,
        std::sin(bet)
    };

    const Vec3 moonEqDateUnit {
        moonEclDate.x,
        std::cos(eps) * moonEclDate.y - std::sin(eps) * moonEclDate.z,
        std::sin(eps) * moonEclDate.y + std::cos(eps) * moonEclDate.z
    };

    Vec3 moonEqDateKm = distanceKm * moonEqDateUnit;

    // Optional topocentric correction in the same equatorial date frame.
    if (observerEqDate) {
        moonEqDateKm = moonEqDateKm - Vec3{observerEqDate->xKm, observerEqDate->yKm, observerEqDate->zKm};
    }

    const double topoDistanceKm = std::max(1.0, norm(moonEqDateKm));
    const Vec3 moonEqDateAu = (1.0 / KM_PER_AU) * moonEqDateKm;

    const SunEquatorial sun = sun_apparent_geocentric(jd_tt);
    const double cosSunDec = std::cos(sun.dec);
    const Vec3 sunEqDateAu {
        std::cos(sun.ra) * cosSunDec * sun.distance_au,
        std::sin(sun.ra) * cosSunDec * sun.distance_au,
        std::sin(sun.dec) * sun.distance_au
    };

    const double phaseDeg = phaseAngleDeg(moonEqDateAu, sunEqDateAu);
    const double magnitude = -12.73 + 0.026 * phaseDeg + 4.0e-9 * std::pow(phaseDeg, 4.0);

    // Convert date-frame equatorial vector back to J2000 so StarCatalog can apply one common PN step.
    double PN[3][3], PNinv[3][3];
    buildPrecessionNutationMatrix(jd_tt, PN);
    transpose3x3(PN, PNinv);

    Vec3 moonEqJ2000 = normalize(mul3x3_3x1(PNinv, moonEqDateAu));

    double ra = std::atan2(moonEqJ2000.y, moonEqJ2000.x);
    if (ra < 0.0)
        ra += 2.0 * M_PI;
    const double dec = std::asin(clampUnit(moonEqJ2000.z));

    BodyEquatorial out;
    out.raRad = ra;
    out.decRad = dec;
    out.distanceAu = topoDistanceKm / KM_PER_AU;
    out.magnitude = magnitude;
    out.angularDiameterRad = 2.0 * std::atan(MOON_RADIUS_KM / topoDistanceKm);
    out.bodyId = BodyId::Moon;
    out.name = "Moon";

    return out;
}

} // namespace astro
