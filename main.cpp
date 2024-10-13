#include <QApplication>
#include "StarMapWidget.h"
#include "StarCatalog.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Загружаем каталог звезд
    StarCatalog catalog("Catalogue.csv");

    // Параметры наблюдателя
    double observer_ra = 1.7421845947129944;  // восхождения наблюдателя (в радианах)
    double observer_dec = -0.3832227200588656; // склонения наблюдателя (в радианах)

    // Поле зрения камеры (в градусах)
    double fovX = 58.0;
    double fovY = 47.0;

    // Получаем звезды, которые видны наблюдателю
    auto stars = catalog.getVisibleStars(observer_ra, observer_dec);

    // Преобразуем их в стереографическую проекцию
    auto [x, y] = catalog.stereographicProjection(stars, observer_ra, observer_dec, fovX, fovY);

    // Создаем виджет для отображения звезд
    StarMapWidget widget(x, y);
    widget.resize(1280, 1038);
    widget.show();

    return app.exec();
}
