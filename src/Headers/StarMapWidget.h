#ifndef STARMAPWIDGET_H
#define STARMAPWIDGET_H

#include <QWidget>
#include <QImage>
#include <vector>

class StarMapWidget : public QWidget {
    Q_OBJECT

public:
    StarMapWidget(
        const std::vector<double>& x,
        const std::vector<double>& y,
        const std::vector<double>& mag,
        QWidget *parent = nullptr
        );

    /** Устанавливает позицию Солнца в проекционных координатах ξ,η */
    void setSunPosition(double xi, double eta);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void renderStars();

    std::vector<double> xCoords;
    std::vector<double> yCoords;
    std::vector<double> magnitudes;

    QImage starMapImage;
    QImage blurredImage;

    // координаты Солнца в системе ξ,η
    double sunXi = 0.0;
    double sunEta = 0.0;
};

#endif // STARMAPWIDGET_H
