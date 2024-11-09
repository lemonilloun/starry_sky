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
    void renderStars();
    QImage starMapImage;    // Изображение для хранения исходного звездного неба
    QImage blurredImage;    // Изображение для хранения размытого звездного неба
    std::vector<double> xCoords;
    std::vector<double> colorIndices;
    std::vector<double> yCoords;
};

#endif // STARMAPWIDGET_H
