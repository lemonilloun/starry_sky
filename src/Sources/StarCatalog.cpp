#include "StarCatalog.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <iostream>  // Для вывода в консоль

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
        star.colorIndex = std::stod(temp);

        stars.push_back(star);
        lineNumber++;
    }

    if (lineNumber == 1) {
        std::cerr << "Ошибка: файл содержит только заголовки или пуст." << std::endl;
    }
}

// =============== Вспомогательные функции для матриц ===============
namespace {
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
    T[0][1] =  X0 / D;
    T[0][2] =  0.0;

    // Вторая строка:
    T[1][0] =  (X0*Z0) / D;
    T[1][1] =  (-Y0*Z0) / D;
    T[1][2] =   D;  // sqrt(X0^2 + Y0^2)

    // Третья строка:
    T[2][0] =  X0;
    T[2][1] =  Y0;
    T[2][2] =  Z0;
}
}

// =========================== Основная логика =============================

std::vector<StarProjection> StarCatalog::projectStars(
    double alpha0, double dec0, double p0,
    double beta1,  double beta2, double p,
    double fovX,   double fovY,
    double maxMagnitude
    ) const
{
    // Шаг 1. Вектор оси визирования r0_eq
    double cosD0 = std::cos(dec0);
    std::array<double,3> r0_eq = {
        std::cos(alpha0)*cosD0,
        std::sin(alpha0)*cosD0,
        std::sin(dec0)
    };

    // Шаг 2. Базовая матрица T_noRoll
    double T_noRoll[3][3];
    buildTransitionMatrix(r0_eq, T_noRoll);

    // Шаг 3. Учитываем p0 (поворот вокруг z)
    double RzP0[3][3], T0[3][3];
    rotationZ(p0, RzP0);
    mul3x3(RzP0, T_noRoll, T0);

    // Шаг 4. Поворот вокруг Oxi, Oeta => Rxy = R_x(beta1) * R_y(beta2).
    double Rx[3][3], Ry[3][3], Rxy[3][3];
    rotationX(beta1, Rx);
    rotationY(beta2, Ry);

    double tmpM[3][3];
    // Порядок: Rxy = R_x * R_y
    mul3x3(Rx, Ry, Rxy);

    // Применяем к (0,0,1)
    std::array<double,3> oldLoS = {0,0,1};
    auto r1_local = mul3x3_3x1(Rxy, oldLoS);

    // Шаг 5. Переходим обратно в экваториальную: r1_eq = T0^-1 * r1_local
    double T0inv[3][3];
    invertOrthogonal(T0, T0inv); // T0 - ортогональная => T0^-1 = T0^T
    auto r1_eq = mul3x3_3x1(T0inv, r1_local);

    // Шаг 6. Строим новую матрицу T1_noRoll (вместо "noRoll", точнее, без учёта p)
    double T1_noRoll[3][3];
    buildTransitionMatrix(r1_eq, T1_noRoll);

    // А теперь обрабатываем звёзды:
    std::vector<StarProjection> projected;
    projected.reserve(stars.size());

    // Шаг 7. Для каждой звезды:
    for (auto &star : stars) {
        // Отбрасываем по величине:
        if (star.id!=0 && star.magnitude > maxMagnitude) {
            continue;
        }

        // Преобразуем RA,Dec -> декартовы (Xeq, Yeq, Zeq)
        double cdec = std::cos(star.dec);
        std::array<double,3> starEq = {
            std::cos(star.ra)*cdec,
            std::sin(star.ra)*cdec,
            std::sin(star.dec)
        };

        // Переходим в систему T1_noRoll: starCam = T1_noRoll * starEq
        auto starCam = mul3x3_3x1(T1_noRoll, starEq);

        // Шаг 8. Вращаем вокруг оси z на угол p:
        double RzP[3][3];
        rotationZ(p, RzP);
        auto starCam2 = mul3x3_3x1(RzP, starCam);

        double x_ = starCam2[0];
        double y_ = starCam2[1];
        double z_ = starCam2[2];

        // ---- новый фильтр по зениту ----
        if (star.id!=0 && z_ <= 0.0)           // угол > 90°  (cosθ ≤ 0) – «за» камерой
            continue;

        // Шаг 9. Проекция: xi = x'/z', eta = y'/z'
        if (std::fabs(z_) < 1e-12) {
            // Звезда на "горизонте" => пропускаем
            continue;
        }
        double xi  = x_ / z_;
        double eta = y_ / z_;

        // Шаг 10. Ограничиваем поле зрения (fovX,fovY).
        // Предположим, fovX,fovY - это "полуширина" в радианах => |xi| < tan(fovX).
        double limitX = std::tan(fovX);
        double limitY = std::tan(fovY);

        if (star.id != 0){
            if (std::fabs(xi) > limitX || std::fabs(eta) > limitY) {
                continue;
            }
        }

        StarProjection pr;
        pr.x         = xi;
        pr.y         = eta;
        pr.magnitude = star.magnitude;
        pr.starId    = star.id;
        projected.push_back(pr);
    }

    for (const auto &pr : projected) {
        std::cout << "Star in FOV => ID=" << pr.starId << std::endl;
    }

    std::cout << "[INFO] projectStars: total = " << stars.size()
              << ", after magnitude => ???, final in fov => "
              << projected.size() << std::endl;

    return projected;
}
