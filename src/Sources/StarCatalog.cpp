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

    // Проверка на успешное открытие файла
    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << filename << std::endl;
        return;
    }

    std::string line;
    int lineNumber = 0;
    std::cout << "Файл успешно открыт: " << filename << std::endl;

    while (std::getline(file, line)) {
        if (lineNumber == 0) {
            // Пропускаем первую строку с заголовками
            lineNumber++;
            continue;
        }

        std::stringstream ss(line);
        Star star;
        std::string temp;

        // Чтение StarID
        std::getline(ss, temp, ',');
        star.id = std::stod(temp); // Преобразуем ID в число

        // Чтение RA
        std::getline(ss, temp, ',');
        star.ra = std::stod(temp) * 15 * M_PI / 180;  // Преобразуем в радианы (RA в часах)

        // Чтение Dec
        std::getline(ss, temp, ',');
        star.dec = std::stod(temp) * M_PI / 180;      // Преобразуем в радианы (Dec в градусах)

        // Чтение Mag
        std::getline(ss, temp, ',');
        star.magnitude = std::stod(temp);             // Преобразуем звездную величину

        // Чтение ColorIndex (можно игнорировать, если не используется)
        std::getline(ss, temp, ',');
        star.colorIndex = std::stod(temp);            // Преобразуем индекс цвета

        stars.push_back(star);  // Добавляем звезду в список

        // Печатаем информацию о считанной звезде для отладки
        //if (lineNumber <= 10) {  // Например, выводим первые 10 звезд
        //    std::cout << "Star " << lineNumber << ": RA = " << star.ra << ", Dec = " << star.dec << ", Mag = " << star.magnitude << std::endl;
        //}

        lineNumber++;
    }

    if (lineNumber == 1) {
        std::cerr << "Ошибка: файл содержит только заголовки или пуст." << std::endl;
    }
}

// Функция для преобразования экваториальных координат в горизонтальные
std::pair<double, double> transformation(double observerDec, double observerRA, double starRA, double starDec) {
    double t = starRA - observerRA;
    double z = acos(sin(starDec) * sin(observerDec) + cos(starDec) * cos(observerDec) * cos(t));

    // Азимут (с учетом нулевых значений)
    double a1 = asin(cos(starDec) * sin(t) / sin(z));
    double a2 = acos((-sin(starDec) * cos(observerDec) + cos(starDec) * sin(observerDec) * cos(t)) / sin(z));
    double azimuth;
    if (a1 > 0) {
        azimuth = a2;
    } else if (a1 < 0) {
        azimuth = 2 * M_PI - a2;
    } else {
        azimuth = 0;
    }

    // Высота светила
    double altitude = M_PI / 2 - z;

    return {altitude, azimuth};
}

// Фильтрация звезд, которые выше горизонта
std::vector<Star> StarCatalog::getVisibleStars(double observerRA, double observerDec, double maxMagnitude) {
    std::vector<Star> visibleStars;

    int totalStars = 0;   // Общее количество обработанных звезд
    int starsAboveHorizon = 0; // Звезды, находящиеся выше горизонта
    int starsByMagnitude = 0;  // Звезды, удовлетворяющие звездной величине

    for (auto& star : stars) {
        // Фильтрация по звездной величине
        if (star.magnitude > maxMagnitude) {
            continue; // Пропускаем звезды, которые слишком тусклые
        }

        starsByMagnitude++;  // Увеличиваем счетчик звезд, подходящих по величине

        // Преобразуем экваториальные координаты в горизонтальные
        auto [altitude, azimuth] = transformation(observerDec, observerRA, star.ra, star.dec);

        totalStars++; // Увеличиваем общее количество звезд

        // Сохраняем горизонтальные координаты в структуру Star
        star.altitude = altitude;
        star.azimuth = azimuth;

        if (altitude > 0) {
            visibleStars.push_back(star);
            starsAboveHorizon++; // Увеличиваем счетчик звезд над горизонтом
        }
    }

    // Выводим информацию о количестве звезд
    std::cout << "Всего обработано: " << totalStars << std::endl;
    std::cout << "Звезд, удовлетворяющих звездной величине: " << starsByMagnitude << std::endl;
    std::cout << "Звезд над горизонтом: " << starsAboveHorizon << std::endl;

    return visibleStars;
}

std::pair<double, double> StarCatalog::adjustProjectionCenter(double theta,
                                                              double psi,
                                                              double phi)
{
    // Исходный центр проекции в горизонтальной системе (A0=0, z0=π/2),
    // в декартовой: (x, y, z) = (0, 1, 0).
    double xc = 0.0, yc = 1.0, zc = 0.0;

    // --- Матрицы поворота вокруг Ox, Oy, Oz --- //
    double cosTheta = std::cos(theta), sinTheta = std::sin(theta);
    double R_x[3][3] = {
        {1,         0,          0},
        {0,  cosTheta,  -sinTheta},
        {0,  sinTheta,   cosTheta}
    };

    double cosPsi = std::cos(psi), sinPsi = std::sin(psi);
    double R_y[3][3] = {
        { cosPsi,  0, sinPsi},
        {      0,  1,     0 },
        {-sinPsi,  0, cosPsi}
    };

    double cosPhi = std::cos(phi), sinPhi = std::sin(phi);
    double R_z[3][3] = {
        { cosPhi, -sinPhi, 0},
        { sinPhi,  cosPhi, 0},
        {      0,       0, 1}
    };

    // --- Подсчитаем, сколько углов ненулевые --- //
    bool isThetaNonZero = (std::fabs(theta) > 1e-12);
    bool isPsiNonZero   = (std::fabs(psi)   > 1e-12);
    bool isPhiNonZero   = (std::fabs(phi)   > 1e-12);
    int  numNonZero     = (int)isThetaNonZero + (int)isPsiNonZero + (int)isPhiNonZero;

    // --- Итоговую матрицу R обнулим для начала --- //
    double R[3][3] = {{0.0, 0.0, 0.0},
                      {0.0, 0.0, 0.0},
                      {0.0, 0.0, 0.0}};

    // --- Функция умножения матриц 3×3 --- //
    auto matMul = [&](const double A[3][3], const double B[3][3], double C[3][3]) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                double sum = 0.0;
                for (int k = 0; k < 3; k++) {
                    sum += A[i][k] * B[k][j];
                }
                C[i][j] = sum;
            }
        }
    };

    if (numNonZero == 1) {
        // --- Случай, когда ровно один угол отличен от нуля --- //
        if (isThetaNonZero) {
            // R = R_x
            std::memcpy(R, R_x, sizeof(R));
        } else if (isPsiNonZero) {
            // R = R_y
            std::memcpy(R, R_y, sizeof(R));
        } else { // isPhiNonZero
            // R = R_z
            std::memcpy(R, R_z, sizeof(R));
        }
    } else {
        // --- Случай, когда либо нет углов, либо несколько --- //
        // Полное умножение R = R_z * R_y * R_x
        double tmp[3][3];
        matMul(R_y, R_x, tmp);  // M = R_y * R_x
        matMul(R_z, tmp, R);    // R = R_z * M
    }

    // --- Применяем итоговое вращение R к вектору центра (xc, yc, zc) --- //
    double x_cd = R[0][0] * xc + R[0][1] * yc + R[0][2] * zc;
    double y_cd = R[1][0] * xc + R[1][1] * yc + R[1][2] * zc;
    double z_cd = R[2][0] * xc + R[2][1] * yc + R[2][2] * zc;

    // --- Обратное преобразование в «горизонтальные» (alt, az) --- //
    // При нашей системе Y - вверх, значит alt = arcsin(Y).
    double newAltitude = std::asin(y_cd);

    // az = atan2(X, Z), с проверкой на "X=Z=0"
    //double newAzimuth = 0.0;
    //if (std::fabs(x_cd) > 1e-12 || std::fabs(z_cd) > 1e-12) {
    //    newAzimuth = std::atan2(x_cd, z_cd);
    //}

    double newAzimuth = std::atan2(x_cd, z_cd);

    return {newAltitude, newAzimuth};
}

std::pair<std::vector<double>, std::vector<double>> StarCatalog::stereographicProjection(
    const std::vector<Star>& stars,
    double fovX,
    double fovY,
    double theta_deg,
    double psi_deg,
    double phi_deg
    ) const
{
    std::vector<double> x, y;


    double theta = theta_deg * M_PI / 180.0;
    double psi   = psi_deg   * M_PI / 180.0;
    double phi   = phi_deg   * M_PI / 180.0;
    auto [centerAlt, centerAz] = StarCatalog::adjustProjectionCenter(theta, psi, phi);

    std::cout << "Новое значения A0: " << centerAz << std::endl;
    std::cout << "Новое значения z0: " << centerAlt << std::endl;

    double A0 = centerAz;
    double z0 = centerAlt;
    // Изначально "рабочий" центр: A0=0, z0= pi/2

    // Углы обзора
    double angleX = (fovX / 2.0) * M_PI / 180.0;
    double angleY = (fovY / 2.0) * M_PI / 180.0;

    double lx = std::abs((sin(z0 + angleX)*cos(z0) - cos(z0+angleX)*sin(z0)*cos(A0)) /
                         (sin(z0)*sin(z0+angleX) + cos(z0+angleX)*cos(z0)*cos(A0)));
    double ly = std::abs((sin(z0 + angleY)*cos(z0) - cos(z0+angleY)*sin(z0)*cos(A0)) /
                         (sin(z0)*sin(z0+angleY) + cos(z0+angleY)*cos(z0)*cos(A0)));

    int starsInFov = 0;

    for (auto& star : stars) {
        double alt = star.altitude;
        double az  = star.azimuth;

        double z = alt;

        double sinZ  = std::sin(z);
        double cosZ  = std::cos(z);
        double cotZ  = (std::fabs(sinZ) < 1e-9) ? 0.0 : (cosZ / sinZ);

        double sinZ0 = std::sin(z0);
        double cosZ0 = std::cos(z0);

        double deltaA = az - A0;
        double denom = sinZ0 + cotZ * cosZ0 * std::cos(deltaA);

        double ksi = (cotZ * std::sin(deltaA)) / denom;
        double eta = (cosZ0 - cotZ * sinZ0 * std::cos(deltaA)) / denom;

        // Проверка FoV
        if (ksi >= -lx && ksi <= lx && eta >= -ly && eta <= ly) {
            x.push_back(ksi);
            y.push_back(eta);
            starsInFov++;
        }
    }

    std::cout << "количество звезд, попавших в поле зрения: " << starsInFov << std::endl;

    return {x, y};
}
