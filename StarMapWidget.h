#ifndef STARMAPWIDGET_H
#define STARMAPWIDGET_H

#include <QWidget>
#include <vector>

class StarMapWidget : public QWidget {
    Q_OBJECT

public:
    StarMapWidget(const std::vector<double>& xCoords, const std::vector<double>& yCoords, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::vector<double> xCoords;
    std::vector<double> yCoords;
};

#endif // STARMAPWIDGET_H
