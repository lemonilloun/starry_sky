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

std::pair<double, double> StarCatalog::adjustProjectionCenter(double theta, double psi, double phi) {
    // Исходный центр проекции в горизонтальной системе:
    // A0 = 0, z0 = π/2. В декартовой системе (при r = 1):
    // x_c = sin(z0)*sin(A0) = 0,
    // y_c = sin(z0)*cos(A0) = 1,
    // z_c = cos(z0) = 0.
    double xc = 0.0, yc = 1.0, zc = 0.0;

    // Матрица поворота вокруг оси Ox (тангаж)
    double cosTheta = std::cos(theta), sinTheta = std::sin(theta);
    double R_x[3][3] = {
        {1, 0, 0},
        {0, cosTheta, -sinTheta},
        {0, sinTheta, cosTheta}
    };

    // Матрица поворота вокруг оси Oy (крен)
    double cosPsi = std::cos(psi), sinPsi = std::sin(psi);
    double R_y[3][3] = {
        {cosPsi, 0, sinPsi},
        {0, 1, 0},
        {-sinPsi, 0, cosPsi}
    };

    // Матрица поворота вокруг оси Oz (рысканье)
    double cosPhi = std::cos(phi), sinPhi = std::sin(phi);
    double R_z[3][3] = {
        {cosPhi, -sinPhi, 0},
        {sinPhi, cosPhi, 0},
        {0, 0, 1}
    };

    // Сначала вычисляем M = R_y * R_x
    double M[3][3] = {0};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            M[i][j] = 0.0;
            for (int k = 0; k < 3; k++) {
                M[i][j] += R_y[i][k] * R_x[k][j];
            }
        }
    }

    // Итоговая матрица R = R_z * M = R_z * (R_y * R_x)
    double R[3][3] = {0};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            R[i][j] = 0.0;
            for (int k = 0; k < 3; k++) {
                R[i][j] += R_z[i][k] * M[k][j];
            }
        }
    }

    // Применяем матрицу R к вектору центра [xc, yc, zc]^T
    double x_cd = R[0][0] * xc + R[0][1] * yc + R[0][2] * zc;
    double y_cd = R[1][0] * xc + R[1][1] * yc + R[1][2] * zc;
    double z_cd = R[2][0] * xc + R[2][1] * yc + R[2][2] * zc;

    // Обратное преобразование в сферические координаты:
    // Интерпретируем ось Y как вертикальную:
    double newAltitude = std::asin(y_cd);           // При (0,1,0): arcsin(1) = π/2.
    double newAzimuth = std::atan2(x_cd, z_cd);       // Если (x_cd, z_cd) = (0,0), можно задать 0 по умолчанию.

    // Если x_cd и z_cd одновременно близки к нулю, установим азимут в 0
    if (std::abs(x_cd) < 1e-9 && std::abs(z_cd) < 1e-9) {
        newAzimuth = 0.0;
    }

    return {newAltitude, newAzimuth};
}

std::pair<std::vector<double>, std::vector<double>> StarCatalog::stereographicProjection(
    const std::vector<Star>& stars,
    double fovX,
    double fovY
    ) const
{
    std::vector<double> x, y;

    // Поворот датчика (theta, psi, phi) -> центр проекции (centerAlt, centerAz)
    double theta = 0.1;
    double psi   = 0.5;
    double phi   = 0.0;
    auto [centerAlt, centerAz] = StarCatalog::adjustProjectionCenter(theta, psi, phi);

    std::cout << "Новое значения A0: " << centerAz << std::endl;
    std::cout << "Новое значения z0: " << centerAlt << std::endl;

    // Теперь A0 = centerAz, z0 = pi/2 - centerAlt
    double A0 = centerAz;
    double z0 = centerAlt;
    // Изначально "рабочий" центр: A0=0, z0= pi/2

    // Углы обзора
    double angleX = (fovX / 2.0) * M_PI / 180.0;
    double angleY = (fovY / 2.0) * M_PI / 180.0;

    // lx, ly - оставим как есть (пока)
    double lx = std::abs((sin(z0 + angleX)*cos(z0) - cos(z0+angleX)*sin(z0)*cos(A0)) /
                         (sin(z0)*sin(z0+angleX) + cos(z0+angleX)*cos(z0)*cos(A0)));
    double ly = std::abs((sin(z0 + angleY)*cos(z0) - cos(z0+angleY)*sin(z0)*cos(A0)) /
                         (sin(z0)*sin(z0+angleY) + cos(z0+angleY)*cos(z0)*cos(A0)));

    int starsInFov = 0;

    for (auto& star : stars) {
        double alt = star.altitude;
        double az  = star.azimuth;

        // Сдвигаем на (A0, z0)
        // Старый код "работал" при A0=0, z0= pi/2 => alt^*= alt, az^*= az
        // Теперь alt^* = alt - ( pi/2 - z0 ) = alt + z0 - pi/2
        // az^* = az - A0
        double altShifted = alt + (z0 - M_PI/2.0);
        double azShifted  = az  - A0;

        // "Рабочая" формула
        // ksi = cos( alt^* ) sin( az^* ) / sin( alt^* )
        // eta = - cos( alt^* ) cos( az^* ) / sin( alt^* )
        double denom = std::sin(altShifted);
        if (std::abs(denom) < 1e-9) {
            continue;
        }
        double ksi = (std::cos(altShifted) * std::sin(azShifted)) / denom;
        double eta = -(std::cos(altShifted) * std::cos(azShifted)) / denom;

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
