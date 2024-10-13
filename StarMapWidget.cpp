#include "StarMapWidget.h"
#include <QPainter>

StarMapWidget::StarMapWidget(const std::vector<double>& xCoords, const std::vector<double>& yCoords, QWidget* parent)
    : QWidget(parent), xCoords(xCoords), yCoords(yCoords) {}

void StarMapWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), Qt::black);  // Фон черного цвета

    painter.setPen(Qt::white);  // Рисуем белые точки

    for (size_t i = 0; i < xCoords.size(); ++i) {
        double screenX = width() / 2 + xCoords[i] * width() / 2;
        double screenY = height() / 2 - yCoords[i] * height() / 2;
        painter.drawEllipse(QPointF(screenX, screenY), 2, 2);  // Рисуем звезду
    }
}
