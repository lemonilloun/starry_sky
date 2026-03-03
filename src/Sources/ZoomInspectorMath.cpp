#include "ZoomInspectorMath.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace {
constexpr double DEG_TO_RAD = M_PI / 180.0;
constexpr double ARCSEC_TO_RAD = DEG_TO_RAD / 3600.0;
constexpr double JULIAN_DAY_J2000 = 2451545.0;
constexpr double OBS_HOUR = 6.0;
constexpr double OBS_MINUTE = 30.0;
constexpr double OBS_SECOND = 5.0;

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

double julianDate(int year, int month, int day,
                  double hour = 0.0,
                  double minute = 0.0,
                  double second = 0.0)
{
    if (month <= 2) {
        year -= 1;
        month += 12;
    }
    const int A = year / 100;
    const int B = 2 - A + A / 4;
    const double dayFraction = (hour + minute / 60.0 + second / 3600.0) / 24.0;
    return std::floor(365.25 * (year + 4716))
         + std::floor(30.6001 * (month + 1))
         + day + dayFraction + B - 1524.5;
}

double normalizeDegrees(double deg)
{
    double res = std::fmod(deg, 360.0);
    if (res < 0.0)
        res += 360.0;
    return res;
}

std::array<double, 3> normalizeVec(const std::array<double, 3>& v)
{
    const double n = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (n <= 1e-15)
        return {0.0, 0.0, 1.0};
    return {v[0] / n, v[1] / n, v[2] / n};
}

std::array<double, 3> mul3x3_3x1(const double M[3][3], const std::array<double, 3>& v)
{
    return {
        M[0][0] * v[0] + M[0][1] * v[1] + M[0][2] * v[2],
        M[1][0] * v[0] + M[1][1] * v[1] + M[1][2] * v[2],
        M[2][0] * v[0] + M[2][1] * v[1] + M[2][2] * v[2]
    };
}

void mul3x3(const double A[3][3], const double B[3][3], double C[3][3])
{
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            C[i][j] = 0.0;
            for (int k = 0; k < 3; ++k)
                C[i][j] += A[i][k] * B[k][j];
        }
    }
}

void transpose3x3(const double A[3][3], double AT[3][3])
{
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            AT[j][i] = A[i][j];
    }
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

void buildTransitionMatrix(const std::array<double, 3>& r0Eq, double T[3][3])
{
    const double X0 = r0Eq[0];
    const double Y0 = r0Eq[1];
    const double Z0 = r0Eq[2];
    const double D2 = X0 * X0 + Y0 * Y0;
    if (D2 < 1e-14) {
        if (Z0 >= 0.0) {
            T[0][0] = 1.0; T[0][1] = 0.0; T[0][2] = 0.0;
            T[1][0] = 0.0; T[1][1] = 1.0; T[1][2] = 0.0;
            T[2][0] = 0.0; T[2][1] = 0.0; T[2][2] = 1.0;
        } else {
            T[0][0] = -1.0; T[0][1] = 0.0;  T[0][2] = 0.0;
            T[1][0] = 0.0;  T[1][1] = -1.0; T[1][2] = 0.0;
            T[2][0] = 0.0;  T[2][1] = 0.0;  T[2][2] = -1.0;
        }
        return;
    }

    const double D = std::sqrt(D2);
    T[0][0] = Y0 / D;          T[0][1] = -X0 / D;         T[0][2] = 0.0;
    T[1][0] = -(X0 * Z0) / D;  T[1][1] = -(Y0 * Z0) / D;  T[1][2] = D;
    T[2][0] = X0;              T[2][1] = Y0;              T[2][2] = Z0;
}

void buildPrecessionMatrix(double t, double P[3][3])
{
    const double t2 = t * t;
    const double t3 = t2 * t;
    const double t4 = t3 * t;
    const double t5 = t4 * t;

    const double ksi =  2.5976176
                      + 2306.0809506 * t
                      + 0.3019015    * t2
                      + 0.0179663    * t3
                      - 0.0000327    * t4
                      - 0.0000002    * t5;
    const double tet =  2004.1917476 * t
                      - 0.4269353    * t2
                      - 0.0418251    * t3
                      - 0.00006018   * t4
                      - 0.0000001    * t5;
    const double z =   -2.5976176
                      + 2306.0803226 * t
                      + 1.0947790    * t2
                      + 0.0182273    * t3
                      + 0.0000470    * t4
                      - 0.0000003    * t5;

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

void computeNutationAngles(double t, double& deltaPsi, double& deltaEps)
{
    double D = normalizeDegrees(297.85036
                                + 445267.111480 * t
                                - 0.0019142 * t * t
                                + t * t * t / 189474.0);
    double M = normalizeDegrees(357.52772
                                + 35999.050340 * t
                                - 0.0001603 * t * t
                                - t * t * t / 300000.0);
    double Mp = normalizeDegrees(134.96298
                                 + 477198.867398 * t
                                 + 0.0086972 * t * t
                                 + t * t * t / 56250.0);
    double F = normalizeDegrees(93.27191
                                + 483202.017538 * t
                                - 0.0036825 * t * t
                                + t * t * t / 327270.0);
    double Omega = normalizeDegrees(125.04452
                                    - 1934.136261 * t
                                    + 0.0020708 * t * t
                                    + t * t * t / 450000.0);

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

    constexpr double SCALE = 1e-4 * ARCSEC_TO_RAD;
    deltaPsi *= SCALE;
    deltaEps *= SCALE;
}

void buildNutationMatrix(double epsMean, double deltaPsi, double deltaEps, double N[3][3])
{
    const double epsTrue = epsMean + deltaEps;
    double R1neg[3][3], R3psi[3][3], R1true[3][3], temp[3][3];
    rotationX(-epsMean, R1neg);
    rotationZ(deltaPsi, R3psi);
    rotationX(epsTrue, R1true);
    mul3x3(R3psi, R1true, temp);
    mul3x3(R1neg, temp, N);
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

bool buildFinalCameraMatrix(const ZoomViewParams& view, double Tfinal[3][3], double PN[3][3])
{
    const double observationJD = julianDate(
        view.obsYear, view.obsMonth, view.obsDay,
        OBS_HOUR, OBS_MINUTE, OBS_SECOND
    );
    buildPrecessionNutationMatrix(observationJD, PN);

    const double cosDec0 = std::cos(view.dec0RadJ2000);
    const std::array<double, 3> r0EqJ2000 = {
        std::cos(view.alpha0RadJ2000) * cosDec0,
        std::sin(view.alpha0RadJ2000) * cosDec0,
        std::sin(view.dec0RadJ2000)
    };
    const auto r0Eq = mul3x3_3x1(PN, r0EqJ2000);

    double TnoRoll[3][3];
    buildTransitionMatrix(r0Eq, TnoRoll);

    double RzP0[3][3], T0[3][3];
    rotationZ(view.p0Rad, RzP0);
    mul3x3(RzP0, TnoRoll, T0);

    double Rx[3][3], Ry[3][3], Rxy[3][3];
    rotationX(view.beta1Rad, Rx);
    rotationY(view.beta2Rad, Ry);
    mul3x3(Rx, Ry, Rxy);

    const std::array<double, 3> oldLoS = {0.0, 0.0, 1.0};
    const auto r1Local = mul3x3_3x1(Rxy, oldLoS);
    double T0inv[3][3];
    transpose3x3(T0, T0inv);
    const auto r1Eq = mul3x3_3x1(T0inv, r1Local);

    double T1noRoll[3][3];
    buildTransitionMatrix(r1Eq, T1noRoll);

    double RzP[3][3];
    rotationZ(view.pRad, RzP);
    mul3x3(RzP, T1noRoll, Tfinal);
    return true;
}
} // namespace

namespace zoominspector {

bool computeNewCenterFromClick(
    const ZoomViewParams& view,
    double clickedXi,
    double clickedEta,
    double& outRaJ2000Rad,
    double& outDecJ2000Rad
)
{
    if (!std::isfinite(clickedXi) || !std::isfinite(clickedEta))
        return false;

    double Tfinal[3][3], PN[3][3];
    if (!buildFinalCameraMatrix(view, Tfinal, PN))
        return false;

    std::array<double, 3> vCam = normalizeVec({clickedXi, clickedEta, 1.0});
    double Tinv[3][3];
    transpose3x3(Tfinal, Tinv);
    auto vEqDate = normalizeVec(mul3x3_3x1(Tinv, vCam));

    double PNinv[3][3];
    transpose3x3(PN, PNinv);
    auto vEqJ2000 = normalizeVec(mul3x3_3x1(PNinv, vEqDate));

    outDecJ2000Rad = std::asin(std::clamp(vEqJ2000[2], -1.0, 1.0));
    outRaJ2000Rad = std::atan2(vEqJ2000[1], vEqJ2000[0]);
    if (outRaJ2000Rad < 0.0)
        outRaJ2000Rad += 2.0 * M_PI;
    return std::isfinite(outRaJ2000Rad) && std::isfinite(outDecJ2000Rad);
}

} // namespace zoominspector
