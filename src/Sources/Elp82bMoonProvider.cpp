#include "Elp82bMoonProvider.h"

#include "SunEphemeris.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace astro {
namespace {

constexpr double PI = 3.141592653589793238462643383279502884;
constexpr double DEG_TO_RAD = PI / 180.0;
constexpr double ARCSEC_TO_RAD = DEG_TO_RAD / 3600.0;
constexpr double JULIAN_DAY_J2000 = 2451545.0;
constexpr double JULIAN_CENTURY_DAYS = 36525.0;
constexpr double RAD_ARCSEC = 648000.0 / PI;
constexpr double KM_PER_AU = 149597870.7;
constexpr double MOON_RADIUS_KM = 1737.4;

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

struct ElpTerm {
    double coeff = 0.0;
    int itab = 1;
    std::array<double, 5> y{{0.0, 0.0, 0.0, 0.0, 0.0}};
};

class Elp82bEngine {
public:
    Elp82bEngine(std::string dataDirectory, double truncationPrecisionRad)
        : m_dataDirectory(std::move(dataDirectory)),
          m_truncationPrecisionRad(truncationPrecisionRad)
    {
        load();
    }

    bool isReady() const { return m_ready; }
    const std::string& error() const { return m_error; }

    bool computeEclipticJ2000Km(double jdTdb, Vec3& outKm) const
    {
        outKm = {};
        if (!m_ready)
            return false;

        const double t1 = (jdTdb - JULIAN_DAY_J2000) / JULIAN_CENTURY_DAYS;
        const double t2 = t1 * t1;
        const double t3 = t2 * t1;
        const double t4 = t3 * t1;

        double r[3] = {0.0, 0.0, 0.0};

        for (int iv = 0; iv < 3; ++iv) {
            double sum = 0.0;
            for (const auto& term : m_terms[iv]) {
                const double y = term.y[0]
                               + term.y[1] * t1
                               + term.y[2] * t2
                               + term.y[3] * t3
                               + term.y[4] * t4;

                double x = term.coeff;
                if (term.itab == 3 || term.itab == 5 || term.itab == 7 || term.itab == 9)
                    x *= t1;
                if (term.itab == 12)
                    x *= t2;

                sum += x * std::sin(y);
            }
            r[iv] = sum;
        }

        const double lon = r[0] / RAD_ARCSEC
            + m_w1[0]
            + m_w1[1] * t1
            + m_w1[2] * t2
            + m_w1[3] * t3
            + m_w1[4] * t4;
        const double lat = r[1] / RAD_ARCSEC;
        const double distKm = r[2] * m_a0 / m_ath;

        double x1 = distKm * std::cos(lat);
        double x2 = x1 * std::sin(lon);
        x1 = x1 * std::cos(lon);
        double x3 = distKm * std::sin(lat);

        const double pw0 = (m_p1 + m_p2 * t1 + m_p3 * t2 + m_p4 * t3 + m_p5 * t4) * t1;
        const double qw0 = (m_q1 + m_q2 * t1 + m_q3 * t2 + m_q4 * t3 + m_q5 * t4) * t1;

        const double ra = 2.0 * std::sqrt(std::max(0.0, 1.0 - pw0 * pw0 - qw0 * qw0));
        const double pwqw = 2.0 * pw0 * qw0;
        const double pw2 = 1.0 - 2.0 * pw0 * pw0;
        const double qw2 = 1.0 - 2.0 * qw0 * qw0;
        const double pw = pw0 * ra;
        const double qw = qw0 * ra;

        outKm.x = pw2 * x1 + pwqw * x2 + pw * x3;
        outKm.y = pwqw * x1 + qw2 * x2 - qw * x3;
        outKm.z = -pw * x1 + qw * x2 + (pw2 + qw2 - 1.0) * x3;
        return true;
    }

private:
    void load()
    {
        m_ready = false;
        m_error.clear();

        namespace fs = std::filesystem;
        fs::path base(m_dataDirectory);
        if (!fs::exists(base)) {
            m_error = "ELP82B directory not found: " + base.string();
            return;
        }

        // Constants from ELP82B_2 reference implementation.
        const double c1 = 60.0;
        const double c2 = 3600.0;
        const double am = 0.074801329518;
        const double alpha = 0.002571881335;
        const double dtasm = 2.0 * alpha / (3.0 * am);

        m_ath = 384747.9806743165;
        m_a0 = 384747.9806448954;

        // Lunar arguments.
        std::array<std::array<double, 5>, 3> w{};
        std::array<double, 5> eart{};
        std::array<double, 5> peri{};

        w[0][0] = (218.0 + 18.0 / c1 + 59.95571 / c2) * DEG_TO_RAD;
        w[1][0] = (83.0 + 21.0 / c1 + 11.67475 / c2) * DEG_TO_RAD;
        w[2][0] = (125.0 + 2.0 / c1 + 40.39816 / c2) * DEG_TO_RAD;
        eart[0] = (100.0 + 27.0 / c1 + 59.22059 / c2) * DEG_TO_RAD;
        peri[0] = (102.0 + 56.0 / c1 + 14.42753 / c2) * DEG_TO_RAD;

        w[0][1] = 1732559343.73604 / RAD_ARCSEC;
        w[1][1] = 14643420.2632 / RAD_ARCSEC;
        w[2][1] = -6967919.3622 / RAD_ARCSEC;
        eart[1] = 129597742.2758 / RAD_ARCSEC;
        peri[1] = 1161.2283 / RAD_ARCSEC;

        w[0][2] = -5.8883 / RAD_ARCSEC;
        w[1][2] = -38.2776 / RAD_ARCSEC;
        w[2][2] = 6.3622 / RAD_ARCSEC;
        eart[2] = -0.0202 / RAD_ARCSEC;
        peri[2] = 0.5327 / RAD_ARCSEC;

        w[0][3] = 0.6604e-2 / RAD_ARCSEC;
        w[1][3] = -0.45047e-1 / RAD_ARCSEC;
        w[2][3] = 0.7625e-2 / RAD_ARCSEC;
        eart[3] = 0.9e-5 / RAD_ARCSEC;
        peri[3] = -0.138e-3 / RAD_ARCSEC;

        w[0][4] = -0.3169e-4 / RAD_ARCSEC;
        w[1][4] = 0.21301e-3 / RAD_ARCSEC;
        w[2][4] = -0.3586e-4 / RAD_ARCSEC;
        eart[4] = 0.15e-6 / RAD_ARCSEC;
        peri[4] = 0.0;

        m_w1 = w[0];

        const double precess = 5029.0966 / RAD_ARCSEC;

        // Planetary arguments.
        std::array<std::array<double, 2>, 8> p{};
        p[0][0] = (252.0 + 15.0 / c1 + 3.25986 / c2) * DEG_TO_RAD;
        p[1][0] = (181.0 + 58.0 / c1 + 47.28305 / c2) * DEG_TO_RAD;
        p[2][0] = eart[0];
        p[3][0] = (355.0 + 25.0 / c1 + 59.78866 / c2) * DEG_TO_RAD;
        p[4][0] = (34.0 + 21.0 / c1 + 5.34212 / c2) * DEG_TO_RAD;
        p[5][0] = (50.0 + 4.0 / c1 + 38.89694 / c2) * DEG_TO_RAD;
        p[6][0] = (314.0 + 3.0 / c1 + 18.01841 / c2) * DEG_TO_RAD;
        p[7][0] = (304.0 + 20.0 / c1 + 55.19575 / c2) * DEG_TO_RAD;

        p[0][1] = 538101628.68898 / RAD_ARCSEC;
        p[1][1] = 210664136.43355 / RAD_ARCSEC;
        p[2][1] = eart[1];
        p[3][1] = 68905077.59284 / RAD_ARCSEC;
        p[4][1] = 10925660.42861 / RAD_ARCSEC;
        p[5][1] = 4399609.65932 / RAD_ARCSEC;
        p[6][1] = 1542481.19393 / RAD_ARCSEC;
        p[7][1] = 786550.32074 / RAD_ARCSEC;

        const double delnu = +0.55604 / RAD_ARCSEC / w[0][1];
        const double dele = +0.01789 / RAD_ARCSEC;
        const double delg = -0.08066 / RAD_ARCSEC;
        const double delnp = -0.06424 / RAD_ARCSEC / w[0][1];
        const double delep = -0.12879 / RAD_ARCSEC;

        std::array<std::array<double, 5>, 4> del{};
        for (int i = 0; i <= 4; ++i) {
            del[0][i] = w[0][i] - eart[i];
            del[3][i] = w[0][i] - w[2][i];
            del[2][i] = w[0][i] - w[1][i];
            del[1][i] = eart[i] - peri[i];
        }
        del[0][0] += 2.0 * PI;

        std::array<double, 2> zeta{};
        zeta[0] = w[0][0];
        zeta[1] = w[0][1] + precess;

        // Precession matrix constants from ELP82B_2.
        m_p1 = 0.10180391e-4;
        m_p2 = 0.47020439e-6;
        m_p3 = -0.5417367e-9;
        m_p4 = -0.2507948e-11;
        m_p5 = 0.463486e-14;
        m_q1 = -0.113469002e-3;
        m_q2 = 0.12372674e-6;
        m_q3 = 0.1265417e-8;
        m_q4 = -0.1371808e-11;
        m_q5 = -0.320334e-14;

        const double prec = m_truncationPrecisionRad;
        const double preVal12 = prec * RAD_ARCSEC - 1e-12;
        const double preVal3 = prec * m_ath - 1e-12;
        const std::array<double, 3> pre{{preVal12, preVal12, preVal3}};

        for (auto& vec : m_terms)
            vec.clear();

        for (int ific = 1; ific <= 36; ++ific) {
            const int itab = (ific + 2) / 3;
            const int iv = ((ific - 1) % 3); // 0..2

            fs::path filePath = base / ("ELP" + std::to_string(ific));
            std::ifstream in(filePath);
            if (!in.is_open()) {
                m_error = "Failed to open ELP file: " + filePath.string();
                return;
            }

            std::string line;
            std::getline(in, line); // header

            while (std::getline(in, line)) {
                if (line.empty())
                    continue;

                std::istringstream iss(line);
                std::vector<double> vals;
                double v = 0.0;
                while (iss >> v)
                    vals.push_back(v);
                if (vals.empty())
                    continue;

                ElpTerm term;
                term.itab = itab;

                if (ific >= 1 && ific <= 3) {
                    if (vals.size() < 11)
                        continue;

                    const int ilu1 = static_cast<int>(std::llround(vals[0]));
                    const int ilu2 = static_cast<int>(std::llround(vals[1]));
                    const int ilu3 = static_cast<int>(std::llround(vals[2]));
                    const int ilu4 = static_cast<int>(std::llround(vals[3]));

                    double c1v = vals[4];
                    const double c2v = vals[5];
                    const double c3v = vals[6];
                    const double c4v = vals[7];
                    const double c5v = vals[8];
                    const double c6v = vals[9];

                    if (std::fabs(c1v) < pre[iv])
                        continue;

                    if (ific == 3)
                        c1v = c1v - 2.0 * c1v * delnu / 3.0;

                    const double tgv = c2v + dtasm * c6v;
                    term.coeff = c1v
                        + tgv * (delnp - am * delnu)
                        + c3v * delg
                        + c4v * dele
                        + c5v * delep;

                    for (int k = 0; k <= 4; ++k) {
                        term.y[k] = ilu1 * del[0][k]
                                  + ilu2 * del[1][k]
                                  + ilu3 * del[2][k]
                                  + ilu4 * del[3][k];
                    }
                    if (iv == 2)
                        term.y[0] += PI / 2.0;
                } else if ((ific >= 4 && ific <= 9) || (ific >= 22 && ific <= 36)) {
                    if (vals.size() < 7)
                        continue;

                    const int iz = static_cast<int>(std::llround(vals[0]));
                    const int ilu1 = static_cast<int>(std::llround(vals[1]));
                    const int ilu2 = static_cast<int>(std::llround(vals[2]));
                    const int ilu3 = static_cast<int>(std::llround(vals[3]));
                    const int ilu4 = static_cast<int>(std::llround(vals[4]));
                    const double pha = vals[5];
                    const double xx = vals[6];

                    if (xx < pre[iv])
                        continue;

                    term.coeff = xx;
                    term.y[0] = pha * DEG_TO_RAD
                              + iz * zeta[0]
                              + ilu1 * del[0][0]
                              + ilu2 * del[1][0]
                              + ilu3 * del[2][0]
                              + ilu4 * del[3][0];
                    term.y[1] = iz * zeta[1]
                              + ilu1 * del[0][1]
                              + ilu2 * del[1][1]
                              + ilu3 * del[2][1]
                              + ilu4 * del[3][1];
                } else { // 10..21
                    if (vals.size() < 13)
                        continue;

                    std::array<int, 11> ipla{};
                    for (int i = 0; i < 11; ++i)
                        ipla[i] = static_cast<int>(std::llround(vals[i]));

                    const double pha = vals[11];
                    const double xx = vals[12];
                    if (xx < pre[iv])
                        continue;

                    term.coeff = xx;
                    for (int k = 0; k <= 1; ++k) {
                        double y = (k == 0) ? pha * DEG_TO_RAD : 0.0;
                        if (ific < 16) {
                            y += ipla[8] * del[0][k] + ipla[9] * del[2][k] + ipla[10] * del[3][k];
                            for (int i = 0; i < 8; ++i)
                                y += ipla[i] * p[i][k];
                        } else {
                            for (int i = 0; i < 4; ++i)
                                y += ipla[i + 7] * del[i][k];
                            for (int i = 0; i < 7; ++i)
                                y += ipla[i] * p[i][k];
                        }
                        term.y[k] = y;
                    }
                }

                m_terms[iv].push_back(term);
            }
        }

        m_ready = true;
    }

private:
    std::string m_dataDirectory;
    double m_truncationPrecisionRad = 0.0;

    bool m_ready = false;
    std::string m_error;

    std::array<std::vector<ElpTerm>, 3> m_terms;

    std::array<double, 5> m_w1{{0.0, 0.0, 0.0, 0.0, 0.0}};
    double m_ath = 384747.9806743165;
    double m_a0 = 384747.9806448954;

    double m_p1 = 0.0;
    double m_p2 = 0.0;
    double m_p3 = 0.0;
    double m_p4 = 0.0;
    double m_p5 = 0.0;
    double m_q1 = 0.0;
    double m_q2 = 0.0;
    double m_q3 = 0.0;
    double m_q4 = 0.0;
    double m_q5 = 0.0;
};

inline double clampUnit(double x)
{
    return std::clamp(x, -1.0, 1.0);
}

double normalizeDegrees(double deg)
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
    const double t = (jd - JULIAN_DAY_J2000) / JULIAN_CENTURY_DAYS;

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

Vec3 eclipticJ2000ToEquatorialJ2000(const Vec3& eclKm)
{
    constexpr double eps = 84381.448 * ARCSEC_TO_RAD;
    const double c = std::cos(eps);
    const double s = std::sin(eps);
    return {
        eclKm.x,
        c * eclKm.y - s * eclKm.z,
        s * eclKm.y + c * eclKm.z
    };
}

double phaseAngleDeg(const Vec3& moonEqDateAu, const Vec3& sunEqDateAu)
{
    const Vec3 moonToSun = sunEqDateAu - moonEqDateAu;
    const Vec3 moonToObserver = -1.0 * moonEqDateAu;
    const double c = clampUnit(dot(normalize(moonToSun), normalize(moonToObserver)));
    return std::acos(c) / DEG_TO_RAD;
}

std::pair<double, double> raDecFromVector(const Vec3& v)
{
    const Vec3 u = normalize(v);
    double ra = std::atan2(u.y, u.x);
    if (ra < 0.0)
        ra += 2.0 * PI;
    const double dec = std::asin(clampUnit(u.z));
    return {ra, dec};
}

const Elp82bEngine* resolveElpEngine(const std::string& dataDir, double precRad, std::string& error)
{
    static std::mutex cacheMutex;
    static std::map<std::string, std::shared_ptr<Elp82bEngine>> cache;

    const std::string key = dataDir + "|" + std::to_string(precRad);
    std::lock_guard<std::mutex> lock(cacheMutex);

    auto it = cache.find(key);
    if (it == cache.end()) {
        auto engine = std::make_shared<Elp82bEngine>(dataDir, precRad);
        it = cache.emplace(key, std::move(engine)).first;
    }

    if (!it->second->isReady()) {
        error = it->second->error();
        return nullptr;
    }

    error.clear();
    return it->second.get();
}

} // namespace

Elp82bMoonProvider::Elp82bMoonProvider(std::string dataDirectory, double truncationPrecisionRad)
    : m_dataDirectory(std::move(dataDirectory)),
      m_truncationPrecisionRad(std::max(0.0, truncationPrecisionRad))
{
}

bool Elp82bMoonProvider::ensureLoaded() const
{
    if (m_loaded)
        return m_ready;

    m_loaded = true;
    std::string err;
    const Elp82bEngine* engine = resolveElpEngine(m_dataDirectory, m_truncationPrecisionRad, err);
    m_ready = (engine != nullptr);
    m_lastError = err;
    return m_ready;
}

bool Elp82bMoonProvider::isReady() const
{
    return ensureLoaded();
}

const std::string& Elp82bMoonProvider::lastError() const
{
    ensureLoaded();
    return m_lastError;
}

BodyEquatorial Elp82bMoonProvider::computeMoon(double jd_tt, const MoonObserverEqDate* observerEqDate) const
{
    BodyEquatorial out;
    out.bodyId = BodyId::Moon;
    out.name = "Moon";

    if (!ensureLoaded()) {
        out.magnitude = 99.0;
        return out;
    }

    std::string err;
    const Elp82bEngine* engine = resolveElpEngine(m_dataDirectory, m_truncationPrecisionRad, err);
    if (!engine) {
        out.magnitude = 99.0;
        return out;
    }

    Vec3 moonEclJ2000Km{};
    if (!engine->computeEclipticJ2000Km(jd_tt, moonEclJ2000Km)) {
        out.magnitude = 99.0;
        return out;
    }

    const Vec3 moonEqJ2000Km = eclipticJ2000ToEquatorialJ2000(moonEclJ2000Km);

    double PN[3][3];
    buildPrecessionNutationMatrix(jd_tt, PN);

    Vec3 moonEqDateKm = mul3x3_3x1(PN, moonEqJ2000Km);
    if (observerEqDate) {
        moonEqDateKm = moonEqDateKm - Vec3{observerEqDate->xKm, observerEqDate->yKm, observerEqDate->zKm};
    }

    const double topoDistanceKm = std::max(1.0, norm(moonEqDateKm));
    const Vec3 moonEqDateAu = (1.0 / KM_PER_AU) * moonEqDateKm;

    const SunEquatorial sun = sun_apparent_geocentric(jd_tt);
    const double cosSunDec = std::cos(sun.dec);
    Vec3 sunEqDateAu {
        std::cos(sun.ra) * cosSunDec * sun.distance_au,
        std::sin(sun.ra) * cosSunDec * sun.distance_au,
        std::sin(sun.dec) * sun.distance_au
    };
    if (observerEqDate) {
        const Vec3 observerAu {
            observerEqDate->xKm / KM_PER_AU,
            observerEqDate->yKm / KM_PER_AU,
            observerEqDate->zKm / KM_PER_AU
        };
        sunEqDateAu = sunEqDateAu - observerAu;
    }

    const double phaseDeg = phaseAngleDeg(moonEqDateAu, sunEqDateAu);
    const double phaseRad = phaseDeg * DEG_TO_RAD;
    const double illumination = std::clamp(0.5 * (1.0 + std::cos(phaseRad)), 0.0, 1.0);

    const double magnitude = -12.73 + 0.026 * phaseDeg + 4.0e-9 * std::pow(phaseDeg, 4.0);

    Vec3 moonEqJ2000Topo = moonEqJ2000Km;
    if (observerEqDate) {
        double PNinv[3][3];
        transpose3x3(PN, PNinv);
        moonEqJ2000Topo = mul3x3_3x1(PNinv, moonEqDateKm);
    }

    const auto [ra, dec] = raDecFromVector(moonEqJ2000Topo);

    out.raRad = ra;
    out.decRad = dec;
    out.distanceAu = topoDistanceKm / KM_PER_AU;
    out.magnitude = magnitude;
    out.angularDiameterRad = 2.0 * std::atan(MOON_RADIUS_KM / topoDistanceKm);
    out.illumination = illumination;
    return out;
}

} // namespace astro
