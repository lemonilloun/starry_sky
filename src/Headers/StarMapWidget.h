#ifndef STARMAPWIDGET_H
#define STARMAPWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QImage>
#include <vector>

class StarMapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StarMapWidget(const std::vector<double>& xCoords,
                           const std::vector<double>& yCoords,
                           const std::vector<double>& magnitudes,
                           const std::vector<double>& colorIndices,
                           QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;      // Отрисовка виджета


private:
    void renderStars();                // Отрисовка звезд

    QImage starMapImage;               // Исходное изображение звездного неба
    QImage blurredImage;               // Размытое изображение
    QPushButton* saveButton;           // Кнопка сохранения изображения
    QPushButton* interactiveModeButton; // Кнопка включения интерактивного режима

    std::vector<double> xCoords;       // Координаты звезд по X
    std::vector<double> yCoords;       // Координаты звезд по Y
    std::vector<double> magnitudes;    // Звездные величины
    std::vector<double> colorIndices;  // Цветовые индексы
};

#endif // STARMAPWIDGET_H
