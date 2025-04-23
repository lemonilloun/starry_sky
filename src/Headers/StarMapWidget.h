#ifndef STARMAPWIDGET_H
#define STARMAPWIDGET_H

#include <QWidget>
#include <QImage>
#include <vector>
#include <cstdint>

class StarMapWidget : public QWidget {
    Q_OBJECT
public:
    // Теперь принимаем ещё вектор ID-шек
    StarMapWidget(
        const std::vector<double>& x,
        const std::vector<double>& y,
        const std::vector<double>& m,
        const std::vector<uint64_t>& ids,
        QWidget* parent = nullptr
        );

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void renderStars();

    std::vector<double>   xCoords;
    std::vector<double>   yCoords;
    std::vector<double>   magnitudes;
    std::vector<uint64_t> starIds;

    QImage starMapImage;
    QImage blurredImage;
};

#endif // STARMAPWIDGET_H
