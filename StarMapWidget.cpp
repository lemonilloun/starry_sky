#include "StarMapWidget.h"
#include "GaussianBlur.h"  // Подключаем заголовочный файл GaussianBlur
#include <QPainter>
#include <QImage>

StarMapWidget::StarMapWidget(const std::vector<double>& xCoords,
                             const std::vector<double>& yCoords,
                             const std::vector<double>& colorIndices,
                             QWidget* parent)
    : QWidget(parent)
    , xCoords(xCoords)
    , yCoords(yCoords)
    , colorIndices(colorIndices)
{
    // Создаем изображение для рисования звездного неба (черно-белое изображение)
    starMapImage = QImage(1280, 1024, QImage::Format_Grayscale8);
    starMapImage.fill(Qt::black);  // Черный фон

    // Рисуем изображение звездного неба
    renderStars();

    // Применяем фильтр Гаусса к изображению звездного неба
    blurredImage = GaussianBlur::applyGaussianBlur(starMapImage, 13, 3.0, 1.5, 0.75);  // Пример параметров: размер ядра, sigmaX, sigmaY, rho
}

void StarMapWidget::renderStars() {
    QPainter painter(&starMapImage);
    painter.setRenderHint(QPainter::Antialiasing);

    for (size_t i = 0; i < xCoords.size(); ++i) {
        // Преобразуем координаты в экранные для QImage
        double screenX = starMapImage.width() / 2 + xCoords[i] * starMapImage.width() / 2;
        double screenY = starMapImage.height() / 2 - yCoords[i] * starMapImage.height() / 2;

        QColor starColor = QColor(255, 255, 255);  // Белый цвет для звезд
        painter.setPen(starColor);
        painter.setBrush(starColor);
        painter.drawEllipse(QPointF(screenX, screenY), 2, 2);  // Размер звезды 2x2 пикселя
    }
}

void StarMapWidget::paintEvent(QPaintEvent* /* event */) {
    QPainter widgetPainter(this);
    // Отображаем изображение с эффектом размытия
    widgetPainter.drawImage(rect(), blurredImage);
}
