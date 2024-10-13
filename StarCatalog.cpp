#include "StarCatalog.h"
#include <fstream>
#include <sstream>
#include <cmath>

StarCatalog::StarCatalog(const std::string& filename) {
    loadFromFile(filename);
}

void StarCatalog::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        Star star;
        std::string temp;

        std::getline(ss, temp, ',');
        star.id = std::stod(temp);
        std::getline(ss, temp, ',');
        star.ra = std::stod(temp) * 15 * M_PI / 180;  // Преобразуем в радианы
        std::getline(ss, temp, ',');
        star.dec = std::stod(temp) * M_PI / 180;      // Преобразуем в радианы
        std::getline(ss, temp, ',');
        star.magnitude = std::stod(temp);

        stars.push_back(star);
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
std::vector<Star> StarCatalog::getVisibleStars(double observerRA, double observerDec) const {
    std::vector<Star> visibleStars;

    for (const auto& star : stars) {
        auto [altitude, azimuth] = transformation(observerDec, observerRA, star.ra, star.dec);
        if (altitude > 0) {
            visibleStars.push_back(star);
        }
    }

    return visibleStars;
}

// Стереографическая проекция с учетом углового поля зрения (FoV)
std::pair<std::vector<double>, std::vector<double>> StarCatalog::stereographicProjection(const std::vector<Star>& stars, double observerRA, double observerDec, double fovX, double fovY ) const {
    std::vector<double> x, y;

    // Углы обзора по X и Y
    double angleX = (fovX / 2) * M_PI / 180;
    double angleY = (fovY / 2) * M_PI / 180;

    for (const auto& star : stars) {
        auto [altitude, azimuth] = transformation(observerDec, observerRA, star.ra, star.dec);

        if (altitude > 0) {
            // Преобразование в ksi и eta (стереографическая проекция)
            double ksi = cos(altitude) * sin(azimuth) / sin(altitude); // x
            double eta = -cos(altitude) * cos(azimuth) / sin(altitude); // y

            // Условия для звёзд, находящихся в пределах кадра
            if (std::abs(ksi) <= tan(angleX) && std::abs(eta) <= tan(angleY)) {
                x.push_back(ksi);
                y.push_back(eta);
            }
        }
    }

    return {x, y};
}
