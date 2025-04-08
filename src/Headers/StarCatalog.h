#ifndef STARCATALOG_H
#define STARCATALOG_H

#include <string>
#include <vector>

// Информация о звезде в каталоге
struct Star {
    double id        = 0.0; // необязательно, если не нужно
    double ra        = 0.0; // в радианах (0..2*pi)
    double dec       = 0.0; // в радианах (-pi/2..+pi/2)
    double magnitude = 99.0;
    double colorIndex= 0.0;
};

// Результат проекции: экранные координаты
struct StarProjection {
    double x;
    double y;
    double magnitude;
    // Можно добавить id или ещё что-то
};

class StarCatalog
{
public:
    explicit StarCatalog(const std::string& filename);
    void loadFromFile(const std::string& filename);

    // Главная функция — новая логика.
    // Углы здесь передаются в радианах (или в градусах — тогда внутри переводить)
    // fovX, fovY — "поле зрения" (полуширина) по x,y.
    // maxMagnitude — максимальная звёздная величина (ярче -> меньше).
    std::vector<StarProjection> projectStars(
        double alpha0, double dec0, double p0,
        double beta1,  double beta2, double p,
        double fovX,   double fovY,
        double maxMagnitude
        ) const;

private:
    std::vector<Star> stars;

    // Можно (необязательно) добавить вспомогательные методы/матрицы
    // в private, чтобы не засорять интерфейс.
};

#endif // STARCATALOG_H
