#ifndef STARCATALOG_H
#define STARCATALOG_H

#include <string>
#include <vector>
#include <array>

// Структура для хранения исходной информации о звезде
struct Star {
    double id = 0.0;        // ID звезды
    double ra = 0.0;        // Прямое восхождение (рад)
    double dec = 0.0;       // Склонение (рад)
    double magnitude = 99.0;
    double colorIndex = 0.0;
};

// Структура для результата проекции
struct StarProjection {
    double x;         // координата на плоскости (после всех поворотов)
    double y;
    double magnitude;
    double starId;
};

class StarCatalog
{
public:
    explicit StarCatalog(const std::string& filename);
    void loadFromFile(const std::string& filename);

    // Новый метод проекции
    std::vector<StarProjection> projectStars(
        double alpha0, double dec0, double p0,
        double beta1,  double beta2, double p,
        double fovX,   double fovY,
        double maxMagnitude
        ) const;

private:
    std::vector<Star> stars;
};

#endif
