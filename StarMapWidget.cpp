#include "StarMapWidget.h"
#include <QPainter>

StarMapWidget::StarMapWidget(const std::vector<double>& xCoords,
                             const std::vector<double>& yCoords,
                             const std::vector<double>& colorIndices,
                             QWidget* parent)
    : QWidget(parent)
    , xCoords(xCoords)
    , yCoords(yCoords)
    , colorIndices(colorIndices) {}

QColor StarMapWidget::getStarColor(double bv) const {
    if (bv < 0.0) {
        return QColor(0, 0, 255);      // blue - Горячие звезды
    } else if (bv >= 0.0 && bv < 0.5) {
        return QColor(173, 216, 230);  // lightblue - Светло-голубой
    } else if (bv >= 0.5 && bv < 1.0) {
        return QColor(255, 255, 0);    // yellow - Желтый
    } else if (bv >= 1.0 && bv < 1.5) {
        return QColor(255, 165, 0);    // orange - Оранжевый
    } else {
        return QColor(255, 0, 0);      // red - Холодные звезды
    }
}

void StarMapWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), Qt::black);  // Черный фон

    for (size_t i = 0; i < xCoords.size(); ++i) {
        double screenX = width() / 2 + xCoords[i] * width() / 2;
        double screenY = height() / 2 - yCoords[i] * height() / 2;

        QColor starColor = QColor(255, 255, 255);
        painter.setPen(starColor);
        painter.setBrush(starColor);
        painter.drawEllipse(QPointF(screenX, screenY), 2, 2);
    }
}
