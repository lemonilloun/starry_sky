#include "StarCatalog.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>  // Для вывода в консоль
#include "CelestialBodyTypes.h"
#include "MoonEphemeris.h"
#include "PlanetEphemeris.h"
#include "SunEphemeris.h"

StarCatalog::StarCatalog(const std::string& filename) {
    loadFromFile(filename);
}

void StarCatalog::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << filename << std::endl;
        return;
    }

    std::string line;
    int lineNumber = 0;
    std::cout << "Файл успешно открыт: " << filename << std::endl;

    while (std::getline(file, line)) {
        if (lineNumber == 0) {
            // Пропускаем заголовок
            lineNumber++;
            continue;
        }

        std::stringstream ss(line);
        Star star;
        std::string temp;

        // Чтение StarID
        std::getline(ss, temp, ',');
        star.id = std::stod(temp);

        // RA (в часах -> перевести в радианы)
        std::getline(ss, temp, ',');
        star.ra = std::stod(temp) * 15.0 * M_PI / 180.0;

        // Dec (в градусах -> радианы)
        std::getline(ss, temp, ',');
        star.dec = std::stod(temp) * M_PI / 180.0;

        // Mag
        std::getline(ss, temp, ',');
        star.magnitude = std::stod(temp);

        // ColorIndex (необязательно)
        std::getline(ss, temp, ',');
        star.colorIndex = temp.empty() ? 0.0 : std::stod(temp);

        // HIP
        std::getline(ss, temp, ',');
        star.hip = temp.empty() ? 0.0 : std::stod(temp);

        // HD
        std::getline(ss, temp, ',');
        star.hd = temp.empty() ? 0.0 : std::stod(temp);

        // HR
        std::getline(ss, temp, ',');
        star.hr = temp.empty() ? 0.0 : std::stod(temp);

        // ProperName
        std::getline(ss, temp, ',');
        star.properName = temp;

        // BayerFlamsteed
        std::getline(ss, temp, ',');
        star.bayerFlamsteed = temp;

        // Gliese
        std::getline(ss, temp, ',');
        star.gliese = temp;

        // SAO (в текущем CSV нет — оставляем пустым)
        star.sao.clear();

        // TYC (в текущем CSV нет — оставляем пустым)
        star.tyc.clear();

        stars.push_back(star);
        lineNumber++;
    }

    if (lineNumber == 1) {
        std::cerr << "Ошибка: файл содержит только заголовки или пуст." << std::endl;
    }
}

// =============== Вспомогательные функции для матриц ===============
namespace {
constexpr uint64_t SUN_ID = astro::bodyIdValue(astro::BodyId::Sun);

std::string trimCopy(std::string s)
{
    auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}

std::string makeDisplayName(const Star& star)
{
    auto trimmedProper = trimCopy(star.properName);
    if (!trimmedProper.empty())
        return trimmedProper;

    auto trimmedBayer = trimCopy(star.bayerFlamsteed);
    if (!trimmedBayer.empty())
        return trimmedBayer;

    auto trimmedGliese = trimCopy(star.gliese);
    if (!trimmedGliese.empty())
        return trimmedGliese;

    if (star.hip > 0.0) {
        long long hipInt = static_cast<long long>(std::llround(star.hip));
        return "HIP " + std::to_string(hipInt);
    }

    if (star.hd > 0.0) {
        long long hdInt = static_cast<long long>(std::llround(star.hd));
        return "HD " + std::to_string(hdInt);
    }

    long long idInt = static_cast<long long>(std::llround(star.id));
    return "Star " + std::to_string(idInt);
}

std::vector<std::string> buildCatalogDesignations(const Star& star, const std::string& primaryName)
{
    std::vector<std::string> records;
    const std::string primaryTrimmed = trimCopy(primaryName);

    auto pushNumber = [&](const char* prefix, double value) {
        if (value <= 0.0)
            return;
        long long id = static_cast<long long>(std::llround(value));
        std::string label = std::string(prefix) + " " + std::to_string(id);
        if (!primaryTrimmed.empty() && trimCopy(label) == primaryTrimmed)
            return;
        records.emplace_back(std::move(label));
    };
    auto pushString = [&](const char* prefix, const std::string& value) {
        auto trimmed = trimCopy(value);
        if (trimmed.empty())
            return;
        std::string label = std::string(prefix) + " " + trimmed;
        if (!primaryTrimmed.empty() && trimCopy(label) == primaryTrimmed)
            return;
        records.emplace_back(std::move(label));
    };

    pushNumber("HIP", star.hip);
    pushNumber("HD",  star.hd);
    pushNumber("HR",  star.hr);
    pushString("Bayer",  star.bayerFlamsteed);
    pushString("Gliese", star.gliese);
    pushString("SAO",    star.sao);
    pushString("TYC",    star.tyc);
    pushNumber("StarID", star.id);
    return records;
}

std::string formatFixed(double value, int precision)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}

std::vector<std::string> buildBodyDesignations(const astro::BodyEquatorial& body)
{
    std::vector<std::string> records;
    records.emplace_back("Type " + astro::bodyTypeLabel(body.bodyId));
    records.emplace_back("DistanceAU " + formatFixed(body.distanceAu, 6));

    if (body.angularDiameterRad > 0.0) {
        double arcsec = body.angularDiameterRad * 180.0 / M_PI * 3600.0;
        records.emplace_back("AngDiamArcsec " + formatFixed(arcsec, 2));
    }

    return records;
}

// Умножение 3×3 на 3×1
std::array<double,3> mul3x3_3x1(const double M[3][3], const std::array<double,3>& v)
{
    std::array<double,3> out;
    for (int i = 0; i < 3; i++) {
        double sum = 0.0;
        for (int j = 0; j < 3; j++) {
            sum += M[i][j] * v[j];
        }
        out[i] = sum;
    }
    return out;
}

// Умножение 3×3 на 3×3
void mul3x3(const double A[3][3], const double B[3][3], double C[3][3])
{
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            double sum = 0.0;
            for (int k = 0; k < 3; k++) {
                sum += A[i][k] * B[k][j];
            }
            C[i][j] = sum;
        }
    }
}

// Транспонирование
void transpose3x3(const double A[3][3], double AT[3][3])
{
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            AT[j][i] = A[i][j];
        }
    }
}

// Так как матрицы ортонормированы, A^-1 = A^T
void invertOrthogonal(const double A[3][3], double Ainv[3][3])
{
    transpose3x3(A, Ainv);
}

// Вращение вокруг оси X
void rotationX(double alpha, double R[3][3])
{
    double c = std::cos(alpha);
    double s = std::sin(alpha);
    R[0][0] = 1;  R[0][1] = 0;  R[0][2] = 0;
    R[1][0] = 0;  R[1][1] = c;  R[1][2] =-s;
    R[2][0] = 0;  R[2][1] = s;  R[2][2] = c;
}

// Вращение вокруг оси Y
void rotationY(double alpha, double R[3][3])
{
    double c = std::cos(alpha);
    double s = std::sin(alpha);
    R[0][0] = c;  R[0][1] = 0; R[0][2] = s;
    R[1][0] = 0;  R[1][1] = 1; R[1][2] = 0;
    R[2][0] =-s;  R[2][1] = 0; R[2][2] = c;
}

// Вращение вокруг оси Z
void rotationZ(double alpha, double R[3][3])
{
    double c = std::cos(alpha);
    double s = std::sin(alpha);
    R[0][0] = c;  R[0][1] =-s; R[0][2] = 0;
    R[1][0] = s;  R[1][1] = c;  R[1][2] = 0;
    R[2][0] = 0;  R[2][1] = 0;  R[2][2] = 1;
}

// По формуле (3.2) — строим матрицу T, переводящую из eq (X,Y,Z) в систему (xi,eta,zeta),
// так что заданный r0_eq (ось визирования) перейдёт в (0,0,1).
void buildTransitionMatrix(const std::array<double,3>& r0_eq, double T[3][3])
{
    double X0 = r0_eq[0];
   double Y0 = r0_eq[1];
   double Z0 = r0_eq[2];

    double D2 = X0*X0 + Y0*Y0;
    //if (D2 < 1e-14) {
        // Случай, когда (X0, Y0) ≈ (0,0), т.е. ось почти параллельна ±Z.
        // Можно сделать либо единичную, либо flip (если Z0 < 0).
    //    if (Z0 > 0) {
    //        T[0][0] = 1; T[0][1] = 0; T[0][2] = 0;
    //        T[1][0] = 0; T[1][1] = 1; T[1][2] = 0;
    //        T[2][0] = 0; T[2][1] = 0; T[2][2] = 1;
    //    } else {
    //        T[0][0] =-1; T[0][1] = 0;  T[0][2] = 0;
    //        T[1][0] = 0; T[1][1] =-1; T[1][2] = 0;
    //        T[2][0] = 0; T[2][1] = 0;  T[2][2] =-1;
    //    }
    //    return;
    //}

    double D = std::sqrt(D2);

    // Первая строка:
    T[0][0] =  Y0 / D;
    T[0][1] = -X0 / D;
    T[0][2] =  0.0;

    // Вторая строка:
    T[1][0] = -(X0*Z0) / D;
    T[1][1] =  (-Y0*Z0) / D;
    T[1][2] =   D;  // sqrt(X0^2 + Y0^2)

    // Третья строка:
    T[2][0] =  X0;
    T[2][1] =  Y0;
    T[2][2] =  Z0;
}

constexpr double DEG_TO_RAD = M_PI / 180.0;
constexpr double ARCSEC_TO_RAD = DEG_TO_RAD / 3600.0;
constexpr double JULIAN_DAY_J2000 = 2451545.0;

double normalizeDegrees(double deg)
{
    double res = std::fmod(deg, 360.0);
    if (res < 0.0)
        res += 360.0;
    return res;
}

double julianDate(int year, int month, int day,
                  double hour = 0.0,
                  double minute = 0.0,
                  double second = 0.0)
{
    if (month <= 2) {
        year  -= 1;
        month += 12;
    }
    int A = year / 100;
    int B = 2 - A + A / 4;

    double dayFraction = (hour + minute / 60.0 + second / 3600.0) / 24.0;
    double jd = std::floor(365.25 * (year + 4716))
              + std::floor(30.6001 * (month + 1))
              + day + dayFraction + B - 1524.5;
    return jd;
}

void buildPrecessionMatrix(double t, double P[3][3])
{
    double t2 = t * t;
    double t3 = t2 * t;
    double t4 = t3 * t;
    double t5 = t4 * t;

    double ksi =  2.5976176
                + 2306.0809506 * t
                + 0.3019015    * t2
                + 0.0179663    * t3
                - 0.0000327    * t4
                - 0.0000002    * t5;

    double tet =  2004.1917476 * t
                - 0.4269353    * t2
                - 0.0418251    * t3
                - 0.00006018   * t4
                - 0.0000001    * t5;

    double z   = -2.5976176
                + 2306.0803226 * t
                + 1.0947790    * t2
                + 0.0182273    * t3
                + 0.0000470    * t4
                - 0.0000003    * t5;

    double rksi = (ksi / 3600.0) * DEG_TO_RAD;
    double rtet = (tet / 3600.0) * DEG_TO_RAD;
    double rz   = (z   / 3600.0) * DEG_TO_RAD;

    double R3ksi[3][3], R2tet[3][3], R3z[3][3];
    rotationZ(-rksi, R3ksi);
    rotationY(rtet,  R2tet);
    rotationZ(-rz,   R3z);

    double temp[3][3];
    mul3x3(R2tet, R3z, temp);
    mul3x3(R3ksi, temp, P);
}

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

double meanObliquity(double t)
{
    double seconds = 84381.406
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
                                - 0.0019142     * t * t
                                + t * t * t / 189474.0);

    double M = normalizeDegrees(357.52772
                                + 35999.050340 * t
                                - 0.0001603    * t * t
                                - t * t * t / 300000.0);

    double Mp = normalizeDegrees(134.96298
                                 + 477198.867398 * t
                                 + 0.0086972     * t * t
                                 + t * t * t / 56250.0);

    double F = normalizeDegrees(93.27191
                                + 483202.017538 * t
                                - 0.0036825     * t * t
                                + t * t * t / 327270.0);

    double Omega = normalizeDegrees(125.04452
                                    - 1934.136261 * t
                                    + 0.0020708   * t * t
                                    + t * t * t / 450000.0);

    D     *= DEG_TO_RAD;
    M     *= DEG_TO_RAD;
    Mp    *= DEG_TO_RAD;
    F     *= DEG_TO_RAD;
    Omega *= DEG_TO_RAD;

    deltaPsi = 0.0;
    deltaEps = 0.0;

    for (const auto& term : NUTATION_TERMS) {
        double argument = term.D * D
                        + term.M * M
                        + term.Mp * Mp
                        + term.F * F
                        + term.Omega * Omega;

        double psiCoeff   = (term.psiCoeff + term.psiCoeffT * t);
        double epsCoeff   = (term.epsCoeff + term.epsCoeffT * t);

        deltaPsi += psiCoeff * std::sin(argument);
        deltaEps += epsCoeff * std::cos(argument);
    }

    constexpr double SCALE = 1e-4 * ARCSEC_TO_RAD;
    deltaPsi *= SCALE;
    deltaEps *= SCALE;
}

void buildNutationMatrix(double epsMean, double deltaPsi, double deltaEps, double N[3][3])
{
    double epsTrue = epsMean + deltaEps;

    double R1_neg_eps[3][3];
    double R3_dpsi[3][3];
    double R1_eps_true[3][3];

    rotationX(-epsMean,  R1_neg_eps);
    rotationZ(deltaPsi,  R3_dpsi);
    rotationX(epsTrue,   R1_eps_true);

    double temp[3][3];
    mul3x3(R3_dpsi, R1_eps_true, temp);
    mul3x3(R1_neg_eps, temp, N);
}

void buildPrecessionNutationMatrix(double jd, double PN[3][3])
{
    double t = (jd - JULIAN_DAY_J2000) / 36525.0;

    double P[3][3];
    buildPrecessionMatrix(t, P);

    double deltaPsi = 0.0;
    double deltaEps = 0.0;
    computeNutationAngles(t, deltaPsi, deltaEps);

    double epsMean = meanObliquity(t);

    double N[3][3];
    buildNutationMatrix(epsMean, deltaPsi, deltaEps, N);

    mul3x3(N, P, PN);
}
}

// =========================== Основная логика =============================

std::vector<StarProjection> StarCatalog::projectStars(
    double alpha0, double dec0, double p0,
    double beta1,  double beta2, double p,
    double fovX,   double fovY,
    double maxMagnitude,
    Sun&   outSun,
    int    obsDay,
    int    obsMonth,
    int    obsYear
    ) const
{
    // --- Prepare the Sun output ---
    outSun.xi    = 0.0;
    outSun.eta   = 0.0;
    outSun.apply = false;

    // Дата наблюдения поступает из UI; время суток пока задаём константами.
    constexpr double OBS_HOUR   = 6.0;
    constexpr double OBS_MINUTE = 30.0;
    constexpr double OBS_SECOND = 5.0;

    double observationJD = julianDate(
        obsYear,
        obsMonth,
        obsDay,
        OBS_HOUR,
        OBS_MINUTE,
        OBS_SECOND
    );

    double PN[3][3];
    buildPrecessionNutationMatrix(observationJD, PN);

    // 1) Compute initial line–of–sight in equatorial coords
    double cosD0 = std::cos(dec0);
    std::array<double,3> r0_eq_j2000 = {
        std::cos(alpha0)*cosD0,
        std::sin(alpha0)*cosD0,
        std::sin(dec0)
    };
    auto r0_eq = mul3x3_3x1(PN, r0_eq_j2000);

    // 2) Build transform T_noRoll
    double T_noRoll[3][3];
    buildTransitionMatrix(r0_eq, T_noRoll);

    // 3) Apply initial roll p0 about Z
    double RzP0[3][3], T0[3][3];
    rotationZ(p0, RzP0);
    mul3x3(RzP0, T_noRoll, T0);

    // 4) Tilt by beta1 around X, beta2 around Y
    double Rx[3][3], Ry[3][3], Rxy[3][3];
    rotationX(beta1, Rx);
    rotationY(beta2, Ry);
    mul3x3(Rx, Ry, Rxy);

    // 5) Compute new equatorial LOS after tilts
    std::array<double,3> oldLoS = {0,0,1};
    auto r1_local = mul3x3_3x1(Rxy, oldLoS);
    double T0inv[3][3];
    invertOrthogonal(T0, T0inv);
    auto r1_eq = mul3x3_3x1(T0inv, r1_local);

    // 6) Build final “no-roll” matrix for new LOS
    double T1_noRoll[3][3];
    buildTransitionMatrix(r1_eq, T1_noRoll);

    // 7) Подготавливаем проекцию
    std::vector<StarProjection> projected;
    projected.reserve(stars.size() + 4);

    double limitX = std::tan(fovX);
    double limitY = std::tan(fovY);

    double canvasW = 1081.0;              // or pull from your QImage width
    double baseRadiusFactor = 1.0;        // same as what you pass to applySunFlare
    double Rpix = std::max(canvasW, canvasW) * baseRadiusFactor;  // =1081
    double dXi   = Rpix / (canvasW / 2.0);  // = 2.0
    double dEta  = Rpix / (canvasW / 2.0);

    double RzP[3][3];
    rotationZ(p, RzP);

    auto appendBodyProjection = [&](const astro::BodyEquatorial& body) {
        if (body.magnitude > maxMagnitude)
            return;

        const double cosDec = std::cos(body.decRad);
        std::array<double, 3> eqVec = {
            std::cos(body.raRad) * cosDec,
            std::sin(body.raRad) * cosDec,
            std::sin(body.decRad)
        };

        auto cam = mul3x3_3x1(T1_noRoll, eqVec);
        auto camRolled = mul3x3_3x1(RzP, cam);
        const double z = camRolled[2];
        if (z <= 0.0 || std::fabs(z) < 1e-12)
            return;

        const double xi = camRolled[0] / z;
        const double eta = camRolled[1] / z;
        if (std::fabs(xi) > limitX || std::fabs(eta) > limitY)
            return;

        StarProjection pr;
        pr.x = xi;
        pr.y = eta;
        pr.magnitude = body.magnitude;
        pr.starId = static_cast<double>(astro::bodyIdValue(body.bodyId));
        pr.raRad = body.raRad;
        pr.decRad = body.decRad;
        pr.displayName = body.name;
        pr.catalogDesignations = buildBodyDesignations(body);
        projected.push_back(pr);
    };

    // Аппарентное положение Солнца на дату наблюдения
    astro::SunEquatorial sunEq = astro::sun_apparent_geocentric(observationJD);
    double cosSunDec = std::cos(sunEq.dec);
    std::array<double,3> sunEqVec = {
        std::cos(sunEq.ra) * cosSunDec,
        std::sin(sunEq.ra) * cosSunDec,
        std::sin(sunEq.dec)
    };
    auto sunCam = mul3x3_3x1(T1_noRoll, sunEqVec);
    auto sunCamRolled = mul3x3_3x1(RzP, sunCam);
    double sunZ = sunCamRolled[2];
    if (sunZ > 0.0 && std::fabs(sunZ) >= 1e-12) {
        double sunXi  = sunCamRolled[0] / sunZ;
        double sunEta = sunCamRolled[1] / sunZ;
        outSun.xi  = sunXi;
        outSun.eta = sunEta;

        double distLeft   = std::fabs( sunXi + limitX );
        double distRight  = std::fabs( limitX - sunXi );
        double distBottom = std::fabs( sunEta + limitY );
        double distTop    = std::fabs( limitY - sunEta );
        if (distLeft   <= dXi ||
            distRight  <= dXi ||
            distBottom <= dEta ||
            distTop    <= dEta)
        {
            outSun.apply = true;
        }

        if (std::fabs(sunXi) <= limitX && std::fabs(sunEta) <= limitY) {
            StarProjection sunProj;
            sunProj.x         = sunXi;
            sunProj.y         = sunEta;
            sunProj.magnitude = -26.74; // видимая звёздная величина Солнца
            sunProj.starId    = SUN_ID;
            sunProj.raRad     = sunEq.ra;
            sunProj.decRad    = sunEq.dec;
            sunProj.displayName = "Sun (Sol)";
            sunProj.catalogDesignations = buildBodyDesignations({
                sunEq.ra,
                sunEq.dec,
                sunEq.distance_au,
                -26.74,
                0.0,
                astro::BodyId::Sun,
                "Sun (Sol)"
            });
            projected.push_back(sunProj);
        }
    }

    // Новые тела (все сразу на дату наблюдения, без дополнительного PN в projectStars()).
    astro::SsdKeplerPlanetProvider planetProvider(/*enableLightTime=*/true, /*iters=*/2);
    astro::MoonLiteProvider moonProvider;
    appendBodyProjection(planetProvider.computeBody(astro::BodyId::Venus, observationJD));
    appendBodyProjection(planetProvider.computeBody(astro::BodyId::Mars, observationJD));
    appendBodyProjection(moonProvider.computeMoon(observationJD));

    for (auto &star : stars) {

        if (star.id == SUN_ID)
            continue;

        if (star.magnitude > maxMagnitude)
            continue;

        // Convert RA/Dec → unit vector in equatorial frame
        double cdec = std::cos(star.dec);
        std::array<double,3> starEq = {
            std::cos(star.ra) * cdec,
            std::sin(star.ra) * cdec,
            std::sin(star.dec)
        };
        auto starEqEpoch = mul3x3_3x1(PN, starEq);

        double ra = std::atan2(starEqEpoch[1], starEqEpoch[0]);
        if (ra < 0.0)
            ra += 2.0 * M_PI;
        double dec = std::asin(std::clamp(starEqEpoch[2], -1.0, 1.0));

        // 7a) Rotate into camera frame (no roll)
        auto starCam = mul3x3_3x1(T1_noRoll, starEqEpoch);

        // 7b) Final roll p around z-axis
        auto starCam2 = mul3x3_3x1(RzP, starCam);

        double x_ = starCam2[0];
        double y_ = starCam2[1];
        double z_ = starCam2[2];

        if (z_ <= 0.0)
            continue;

        if (std::fabs(z_) < 1e-12)
            continue;
        double xi  = x_ / z_;
        double eta = y_ / z_;

        if (std::fabs(xi) > limitX || std::fabs(eta) > limitY)
            continue;

        StarProjection pr;
        pr.x         = xi;
        pr.y         = eta;
        pr.magnitude = star.magnitude;
        pr.starId    = star.id;
        pr.raRad     = ra;
        pr.decRad    = dec;
        pr.displayName = makeDisplayName(star);
        pr.catalogDesignations = buildCatalogDesignations(star, pr.displayName);
        projected.push_back(pr);
    }

    return projected;
}
