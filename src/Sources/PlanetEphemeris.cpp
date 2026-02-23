#include "PlanetEphemeris.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace astro {
namespace {

constexpr double DEG_TO_RAD = M_PI / 180.0;
constexpr double ARCSEC_TO_RAD = DEG_TO_RAD / 3600.0;
constexpr double JULIAN_DAY_J2000 = 2451545.0;
// SSD "Approximate Positions of the Planets" uses epsilon = 23.43928 deg at J2000.
constexpr double EPSILON_J2000 = 23.43928 * DEG_TO_RAD;
constexpr double C_AU_PER_DAY = 173.1446326846693;
constexpr double KM_PER_AU = 149597870.7;

struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct KeplerCoefficients {
    double a0;
    double a1;
    double e0;
    double e1;
    double i0Deg;
    double i1Deg;
    double l0Deg;
    double l1Deg;
    double varpi0Deg;
    double varpi1Deg;
    double omega0Deg;
    double omega1Deg;
};

struct NutationTerm {
    int D;
    int M;
    int Mp;
    int F;
    int Omega;
    double psiCoeff;
    double psiCoeffT;
    double epsCoeff;
    double epsCoeffT;
};

constexpr KeplerCoefficients EMB_COEFF {
    1.00000261,  0.00000562,
    0.01671123, -0.00004392,
   -0.00001531, -0.01294668,
  100.46457166, 35999.37244981,
  102.93768193, 0.32327364,
    0.0, 0.0
};

constexpr KeplerCoefficients VENUS_COEFF {
    0.72333566,  0.00000390,
    0.00677672, -0.00004107,
    3.39467605, -0.00078890,
  181.97909950, 58517.81538729,
  131.60246718, 0.00268329,
   76.67984255, -0.27769418
};

constexpr KeplerCoefficients MARS_COEFF {
    1.52371034, 0.00001847,
    0.09339410, 0.00007882,
    1.84969142, -0.00813131,
   -4.55343205, 19140.30268499,
  -23.94362959, 0.44441088,
   49.55953891, -0.29257343
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

inline double clampUnit(double x)
{
    return std::clamp(x, -1.0, 1.0);
}

double normDeg(double deg)
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

Vec3 rotateEclToEqJ2000(const Vec3& ecl)
{
    const double c = std::cos(EPSILON_J2000);
    const double s = std::sin(EPSILON_J2000);
    return {
        ecl.x,
        c * ecl.y - s * ecl.z,
        s * ecl.y + c * ecl.z
    };
}

Vec3 rotateX(const Vec3& v, double a)
{
    const double c = std::cos(a);
    const double s = std::sin(a);
    return {
        v.x,
        c * v.y - s * v.z,
        s * v.y + c * v.z
    };
}

Vec3 rotateZ(const Vec3& v, double a)
{
    const double c = std::cos(a);
    const double s = std::sin(a);
    return {
        c * v.x - s * v.y,
        s * v.x + c * v.y,
        v.z
    };
}

double solveKepler(double meanAnomalyRad, double e)
{
    double M = std::fmod(meanAnomalyRad, 2.0 * M_PI);
    if (M < -M_PI)
        M += 2.0 * M_PI;
    if (M > M_PI)
        M -= 2.0 * M_PI;

    double E = (e < 0.8) ? M : M_PI;
    for (int i = 0; i < 30; ++i) {
        const double f = E - e * std::sin(E) - M;
        const double fp = 1.0 - e * std::cos(E);
        const double dE = f / fp;
        E -= dE;
        if (std::fabs(dE) <= 1e-12)
            break;
    }
    return E;
}

Vec3 heliocentricEclipticFromCoefficients(const KeplerCoefficients& coeff, double jd_tt)
{
    const double T = (jd_tt - JULIAN_DAY_J2000) / 36525.0;

    const double a = coeff.a0 + coeff.a1 * T;
    const double e = coeff.e0 + coeff.e1 * T;
    const double I = (coeff.i0Deg + coeff.i1Deg * T) * DEG_TO_RAD;
    const double L = normDeg(coeff.l0Deg + coeff.l1Deg * T) * DEG_TO_RAD;
    const double varpi = normDeg(coeff.varpi0Deg + coeff.varpi1Deg * T) * DEG_TO_RAD;
    const double Omega = normDeg(coeff.omega0Deg + coeff.omega1Deg * T) * DEG_TO_RAD;

    const double omega = varpi - Omega;
    const double M = L - varpi;
    const double E = solveKepler(M, e);

    const double xPrime = a * (std::cos(E) - e);
    const double yPrime = a * std::sqrt(std::max(0.0, 1.0 - e * e)) * std::sin(E);

    Vec3 orbital {xPrime, yPrime, 0.0};

    // r_ecl = Rz(Omega) * Rx(I) * Rz(omega) * r_orbital
    Vec3 ecl = rotateZ(orbital, omega);
    ecl = rotateX(ecl, I);
    ecl = rotateZ(ecl, Omega);
    return ecl;
}

double meanObliquityRad(double t)
{
    const double t2 = t * t;
    const double t3 = t2 * t;
    const double t4 = t3 * t;
    const double t5 = t4 * t;
    const double seconds = 84381.406
        - 46.836769 * t
        - 0.0001831 * t2
        + 0.00200340 * t3
        - 0.000000576 * t4
        - 0.0000000434 * t5;
    return seconds * ARCSEC_TO_RAD;
}

void rotationXMatrix(double alpha, double R[3][3])
{
    const double c = std::cos(alpha);
    const double s = std::sin(alpha);
    R[0][0] = 1.0; R[0][1] = 0.0; R[0][2] = 0.0;
    R[1][0] = 0.0; R[1][1] = c;   R[1][2] = -s;
    R[2][0] = 0.0; R[2][1] = s;   R[2][2] = c;
}

void rotationYMatrix(double alpha, double R[3][3])
{
    const double c = std::cos(alpha);
    const double s = std::sin(alpha);
    R[0][0] = c;   R[0][1] = 0.0; R[0][2] = s;
    R[1][0] = 0.0; R[1][1] = 1.0; R[1][2] = 0.0;
    R[2][0] = -s;  R[2][1] = 0.0; R[2][2] = c;
}

void rotationZMatrix(double alpha, double R[3][3])
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
    rotationZMatrix(-rksi, R3ksi);
    rotationYMatrix(rtet, R2tet);
    rotationZMatrix(-rz, R3z);
    mul3x3(R2tet, R3z, temp);
    mul3x3(R3ksi, temp, P);
}

void computeNutationAngles(double t, double& deltaPsi, double& deltaEps)
{
    double D = normDeg(297.85036 + 445267.111480 * t - 0.0019142 * t * t + (t * t * t) / 189474.0);
    double M = normDeg(357.52772 + 35999.050340 * t - 0.0001603 * t * t - (t * t * t) / 300000.0);
    double Mp = normDeg(134.96298 + 477198.867398 * t + 0.0086972 * t * t + (t * t * t) / 56250.0);
    double F = normDeg(93.27191 + 483202.017538 * t - 0.0036825 * t * t + (t * t * t) / 327270.0);
    double Omega = normDeg(125.04452 - 1934.136261 * t + 0.0020708 * t * t + (t * t * t) / 450000.0);

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
    double R1neg[3][3], R3psi[3][3], R1true[3][3], temp[3][3];
    rotationXMatrix(-epsMean, R1neg);
    rotationZMatrix(deltaPsi, R3psi);
    rotationXMatrix(epsTrue, R1true);
    mul3x3(R3psi, R1true, temp);
    mul3x3(R1neg, temp, N);
}

Vec3 applyPrecessionNutation(const Vec3& eqJ2000, double jd_tt)
{
    const double t = (jd_tt - JULIAN_DAY_J2000) / 36525.0;

    double P[3][3];
    buildPrecessionMatrix(t, P);

    double deltaPsi = 0.0;
    double deltaEps = 0.0;
    computeNutationAngles(t, deltaPsi, deltaEps);

    const double epsMean = meanObliquityRad(t);
    double N[3][3], PN[3][3];
    buildNutationMatrix(epsMean, deltaPsi, deltaEps, N);
    mul3x3(N, P, PN);

    return mul3x3_3x1(PN, eqJ2000);
}

std::pair<double, double> raDecFromVector(const Vec3& eq)
{
    const Vec3 n = normalize(eq);
    double ra = std::atan2(n.y, n.x);
    if (ra < 0.0)
        ra += 2.0 * M_PI;
    const double dec = std::asin(clampUnit(n.z));
    return {ra, dec};
}

double phaseAngleDeg(const Vec3& planetHelioEcl, const Vec3& earthHelioEcl)
{
    const Vec3 planetToSun = -1.0 * planetHelioEcl;
    const Vec3 planetToEarth = earthHelioEcl - planetHelioEcl;
    const double c = clampUnit(dot(normalize(planetToSun), normalize(planetToEarth)));
    return std::acos(c) / DEG_TO_RAD;
}

double magnitudeForPlanet(BodyId bodyId, double helioDistanceAu, double geoDistanceAu, double phaseDeg)
{
    if (helioDistanceAu <= 0.0 || geoDistanceAu <= 0.0)
        return 99.0;

    if (bodyId == BodyId::Venus) {
        return -4.40
            + 5.0 * std::log10(helioDistanceAu * geoDistanceAu)
            + 0.0009 * phaseDeg
            + 0.000239 * phaseDeg * phaseDeg
            - 0.00000065 * phaseDeg * phaseDeg * phaseDeg;
    }

    if (bodyId == BodyId::Mars) {
        return -1.52
            + 5.0 * std::log10(helioDistanceAu * geoDistanceAu)
            + 0.016 * phaseDeg;
    }

    return 99.0;
}

double radiusKmForBody(BodyId bodyId)
{
    if (bodyId == BodyId::Venus)
        return 6051.8;
    if (bodyId == BodyId::Mars)
        return 3396.19;
    return 0.0;
}

const KeplerCoefficients& coefficientsForBody(BodyId bodyId)
{
    switch (bodyId) {
    case BodyId::Venus:
        return VENUS_COEFF;
    case BodyId::Mars:
        return MARS_COEFF;
    default:
        break;
    }
    throw std::invalid_argument("Unsupported body for SsdKeplerPlanetProvider");
}

std::string bodyName(BodyId bodyId)
{
    if (bodyId == BodyId::Venus)
        return "Venus";
    if (bodyId == BodyId::Mars)
        return "Mars";
    return "";
}

} // namespace

SsdKeplerPlanetProvider::SsdKeplerPlanetProvider(bool enableLightTime, int lightTimeIterations)
    : m_enableLightTime(enableLightTime),
      m_lightTimeIterations(std::max(0, lightTimeIterations))
{
}

BodyEquatorial SsdKeplerPlanetProvider::computeBody(BodyId bodyId, double jd_tt) const
{
    const auto& targetCoeff = coefficientsForBody(bodyId);

    const Vec3 earthHelioObs = heliocentricEclipticFromCoefficients(EMB_COEFF, jd_tt);

    double jdPlanet = jd_tt;
    Vec3 planetHelio = heliocentricEclipticFromCoefficients(targetCoeff, jdPlanet);
    Vec3 rhoEcl = planetHelio - earthHelioObs;

    if (m_enableLightTime) {
        for (int i = 0; i < m_lightTimeIterations; ++i) {
            const double deltaAu = norm(rhoEcl);
            jdPlanet = jd_tt - deltaAu / C_AU_PER_DAY;
            planetHelio = heliocentricEclipticFromCoefficients(targetCoeff, jdPlanet);
            rhoEcl = planetHelio - earthHelioObs;
        }
    }

    const Vec3 rhoEqJ2000 = rotateEclToEqJ2000(rhoEcl);
    const auto [ra, dec] = raDecFromVector(rhoEqJ2000);

    const double helioDistanceAu = norm(planetHelio);
    const double geoDistanceAu = norm(rhoEcl);
    const double phaseDeg = phaseAngleDeg(planetHelio, earthHelioObs);
    const double phaseRad = phaseDeg * DEG_TO_RAD;

    BodyEquatorial out;
    out.raRad = ra;
    out.decRad = dec;
    out.distanceAu = geoDistanceAu;
    out.magnitude = magnitudeForPlanet(bodyId, helioDistanceAu, geoDistanceAu, phaseDeg);
    out.illumination = std::clamp(0.5 * (1.0 + std::cos(phaseRad)), 0.0, 1.0);
    out.bodyId = bodyId;
    out.name = bodyName(bodyId);

    const double radiusKm = radiusKmForBody(bodyId);
    if (radiusKm > 0.0 && geoDistanceAu > std::numeric_limits<double>::epsilon()) {
        out.angularDiameterRad = 2.0 * std::atan(radiusKm / (geoDistanceAu * KM_PER_AU));
    }

    return out;
}

} // namespace astro
