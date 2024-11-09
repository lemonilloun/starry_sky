#ifndef STARMAPWIDGET_H
#define STARMAPWIDGET_H

#include <QWidget>
#include <QImage>
#include <vector>

class StarMapWidget : public QWidget {
    Q_OBJECT
public:
    StarMapWidget(const std::vector<double>& xCoords,
                  const std::vector<double>& yCoords,
                  const std::vector<double>& colorIndices,
                  QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    // Метод для рендеринга звезд на изображении
    void renderStars();

    // Хранение звездного неба в виде изображения
    QImage starMapImage;

    // Координаты и цветовые индексы звезд
    std::vector<double> xCoords;
    std::vector<double> yCoords;
    std::vector<double> colorIndices;
};

#endif // STARMAPWIDGET_H
