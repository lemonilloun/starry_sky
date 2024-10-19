#ifndef STARCATALOG_H
#define STARCATALOG_H

#include <vector>
#include <string>
#include <utility>

// Структура для представления информации о звезде
struct Star {
    double id;          // Идентификатор звезды
    double ra;          // Прямое восхождение (в радианах)
    double dec;         // Склонение (в радианах)
    double magnitude;   // Звездная величина
    double colorIndex;  // Индекс цвета (можно игнорировать, если не используется)

    // Горизонтальные координаты
    double altitude;    // Высота
    double azimuth;     // Азимут
};

// Класс для работы с каталогом звезд
class StarCatalog {
public:
    // Конструктор, принимающий имя файла CSV для загрузки данных
    StarCatalog(const std::string& filename);

    // Метод для получения звезд, видимых наблюдателю с заданными координатами
    std::vector<Star> getVisibleStars(double observerRA, double observerDec, double maxMagnitude);

    // Метод для преобразования видимых звезд в стереографическую проекцию с учетом поля зрения (FoV)
    std::pair<std::vector<double>, std::vector<double>> stereographicProjection(
        const std::vector<Star>& stars,
        double fovX,
        double fovY
        ) const;

private:
    // Вектор для хранения данных о звездах из каталога
    std::vector<Star> stars;

    // Метод для загрузки данных звезд из файла CSV
    void loadFromFile(const std::string& filename);
};

#endif // STARCATALOG_H
