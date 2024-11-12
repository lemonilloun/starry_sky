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
                  const std::vector<double>& magnitudes,   // Добавляем magnitudes
                  const std::vector<double>& colorIndices,
                  QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void renderStars();
    QImage starMapImage;           // Изображение для хранения исходного звездного неба
    QImage blurredImage;           // Изображение для хранения размытого звездного неба
    std::vector<double> xCoords;   // Координаты по оси X
    std::vector<double> yCoords;   // Координаты по оси Y
    std::vector<double> magnitudes; // Добавляем вектор magnitudes для звездной величины
    std::vector<double> colorIndices; // Вектор цветовых индексов звезд
};

#endif // STARMAPWIDGET_H
