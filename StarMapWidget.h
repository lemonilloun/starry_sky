#ifndef STARMAPWIDGET_H
#define STARMAPWIDGET_H
#include <QWidget>
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
    QColor getStarColor(double colorIndex) const;
    std::vector<double> xCoords;
    std::vector<double> yCoords;
    std::vector<double> colorIndices;
};
#endif // STARMAPWIDGET_H
