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

// Проекция в прямоугольные координаты с учетом углового поля зрения (FoV)
std::pair<std::vector<double>, std::vector<double>> StarCatalog::stereographicProjection(
    const std::vector<Star>& stars,
    double fovX,
    double fovY
    ) const {
    std::vector<double> x, y;

    // Углы обзора по X и Y в радианах
    double angleX = (fovX / 2) * M_PI / 180;
    double angleY = (fovY / 2) * M_PI / 180;

    // Расчет границ кадра lx и ly
    double lx = std::abs((sin(M_PI / 2 + angleX) * cos(M_PI / 2) - cos(M_PI / 2 + angleX) * sin(M_PI / 2) * cos(0)) /
                         (sin(M_PI / 2) * sin(M_PI / 2 + angleX) + cos(M_PI / 2 + angleX) * cos(M_PI / 2) * cos(0)));

    double ly = std::abs((sin(M_PI / 2 + angleY) * cos(M_PI / 2) - cos(M_PI / 2 + angleY) * sin(M_PI / 2) * cos(0)) /
                         (sin(M_PI / 2) * sin(M_PI / 2 + angleY) + cos(M_PI / 2 + angleY) * cos(M_PI / 2) * cos(0)));

    std::cout << "lx: " << lx << " ly: " << ly << std::endl;

    int starsInFov = 0;  // Счетчик звезд, попавших в поле зрения

    for (const auto& star : stars) {
        // Используем уже сохраненные горизонтальные координаты (altitude и azimuth)
        double altitude = star.altitude;
        double azimuth = star.azimuth;

        // Преобразуем горизонтальные координаты в стереографические
        double ksi = cos(altitude) * sin(azimuth) / sin(altitude); // x
        double eta = -cos(altitude) * cos(azimuth) / sin(altitude); // y

        // Проверяем, находится ли звезда в пределах границ кадра
        if (ksi >= -lx && ksi <= lx && eta >= -ly && eta <= ly) {
            x.push_back(ksi);
            y.push_back(eta);
            starsInFov++;
        }
    }

    std::cout << "количество звезд, попавших в поле зрения: " << starsInFov << std::endl;

    // Ширина и высота виджета
    constexpr double widgetWidth = 1280.0;
    constexpr double widgetHeight = 1024.0;

    // Масштабирование координат
    double scaleX = 0.7 / (lx);  // Масштаб по ширине
    double scaleY = 0.7 / (ly); // Масштаб по высоте

    // Применяем масштабирование к координатам
    for (size_t i = 0; i < x.size(); ++i) {
        x[i] = (x[i] * scaleX);  // Пропорциональное масштабирование и сдвиг в центр
        y[i] = (y[i] * scaleY); // Пропорциональное масштабирование и сдвиг в центр
    }

    return {x, y};
}
