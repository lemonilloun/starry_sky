#include <QApplication>
#include <iostream>
#include "StarMapWidget.h"
#include "StarCatalog.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Загружаем каталог звезд
    StarCatalog catalog("/Users/lehacho/starry_sky/Catalogue_clean.csv");

    // Параметры наблюдателя
    double observer_ra = 1.7421845947129944
;  // Пример восхождения наблюдателя (в радианах)
    double observer_dec = -0.3832227200588656; // Пример склонения наблюдателя (в радианах)

    // Максимальная видимая звездная величина
    double maxMagnitude = 3.8;  // Например, ограничение для видимых звезд

    // Поле зрения камеры (в градусах)
    double fovX = 58.0;
    double fovY = 47.0;

    // Получаем звезды, которые видны наблюдателю
    auto visibleStars = catalog.getVisibleStars(observer_ra, observer_dec, maxMagnitude);

    // Выводим количество звезд над горизонтом
    std::cout << "Количество звезд над горизонтом: " << visibleStars.size() << std::endl;

    // Преобразуем их в стереографическую проекцию и проверяем, сколько звезд попадает в поле зрения
    auto [x, y] = catalog.stereographicProjection(visibleStars, fovX, fovY);

    std::vector<double> colorIndices;
    colorIndices.reserve(visibleStars.size());
    for (const auto& star : visibleStars) {
        colorIndices.push_back(star.colorIndex);  // Предполагается, что в структуре
    }

    // Создаем виджет для отображения звезд
    StarMapWidget widget(x, y, colorIndices);
    widget.resize(1280, 1038);
    widget.show();

    return app.exec();
}
