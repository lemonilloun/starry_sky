#include "StarMapWidget.h"
#include "GaussianBlur.h"
#include "LightPollution.h"
#include <QPainter>
#include <QImage>
#include <QFileDialog>
#include <QMessageBox>
#include <QWheelEvent>
#include <QVBoxLayout>

StarMapWidget::StarMapWidget(const std::vector<double>& xCoords,
                             const std::vector<double>& yCoords,
                             const std::vector<double>& magnitudes,
                             const std::vector<double>& colorIndices,
                             QWidget* parent)
    : QWidget(parent)
    , xCoords(xCoords)
    , yCoords(yCoords)
    , magnitudes(magnitudes)
    , colorIndices(colorIndices)
{
    // Создаем изображение для рисования звездного неба (черно-белое изображение)
    starMapImage = QImage(1280, 1024, QImage::Format_Grayscale8);
    starMapImage.fill(Qt::black);  // Черный фон

    // Рисуем изображение звездного неба
    renderStars();

    // Применяем фильтр Гаусса к изображению звездного неба
    blurredImage = GaussianBlur::applyGaussianBlur(starMapImage, 13, 2.0, 1.0, 0.60);  // гиперпараметры: размер ядра, sigmaX, sigmaY, rho
    blurredImage = LightPollution::applyLightPollution(blurredImage, 0.0, 0.8);       // гиперпараметры: intensity, gradientFactor

    // Создаем кнопки
    saveButton = new QPushButton("Save Image", this);
    interactiveModeButton = new QPushButton("Interactive Mode", this);

    // Устанавливаем размеры и начальную позицию кнопок
    saveButton->setFixedSize(120, 40);
    interactiveModeButton->setFixedSize(140, 40);

    saveButton->move(width() - saveButton->width() - 10, 10); // Справа сверху
    interactiveModeButton->move(10, 10); // Слева сверху

    // Привязываем сигналы кнопок
    connect(saveButton, &QPushButton::clicked, this, [this]() {
        QString filePath = QFileDialog::getSaveFileName(this, "Save Image", "", "PNG Files (*.png);;JPEG Files (*.jpg)");
        if (!filePath.isEmpty()) {
            blurredImage.save(filePath);
            qDebug() << "Image saved to:" << filePath;
        }
    });

    connect(interactiveModeButton, &QPushButton::clicked, this, [this]() {
        qDebug() << "Interactive Mode clicked";
        // Здесь реализуйте логику включения интерактивного режима
    });
}

void StarMapWidget::renderStars() {
    QPainter painter(&starMapImage);
    painter.setRenderHint(QPainter::Antialiasing);

    const double brightnessScale = 5.0;  // Коэффициент усиления яркости
    const int minBrightness = 30;        // Минимальная яркость, чтобы звезды не были слишком тусклыми
    const double maxStarRadius = 3.0;    // Максимальный радиус звезды

    for (size_t i = 0; i < xCoords.size(); ++i) {
        double screenX = starMapImage.width() / 2 + xCoords[i] * starMapImage.width() / 2;
        double screenY = starMapImage.height() / 2 - yCoords[i] * starMapImage.height() / 2;

        double brightnessFactor = brightnessScale / std::pow(2.512, magnitudes[i]);
        int brightness = static_cast<int>(std::clamp(brightnessFactor * 255, 0.0, 255.0));

        brightness = std::max(brightness, minBrightness);

        QColor starColor(brightness, brightness, brightness);
        painter.setPen(starColor);
        painter.setBrush(starColor);

        double starRadius = std::clamp(1.5 * std::sqrt(brightnessFactor), 1.0, maxStarRadius);
        painter.drawEllipse(QPointF(screenX, screenY), starRadius, starRadius);
    }
}

void StarMapWidget::paintEvent(QPaintEvent* /* event */) {
    QPainter widgetPainter(this);

    if (isInteractiveMode) {
        // Отображаем увеличенное/уменьшенное изображение в режиме интеракции
        QRect scaledRect = rect();
        scaledRect.setWidth(rect().width() * scaleFactor);
        scaledRect.setHeight(rect().height() * scaleFactor);
        widgetPainter.drawImage(rect(), blurredImage, scaledRect);
    } else {
        // Отображаем изображение без интеракции
        widgetPainter.drawImage(rect(), blurredImage);
    }
}

void StarMapWidget::wheelEvent(QWheelEvent* event) {
    if (!isInteractiveMode) {
        QWidget::wheelEvent(event); // Если не интерактивный режим, игнорируем колесико
        return;
    }

    // Изменяем масштаб в зависимости от направления прокрутки
    double zoomFactor = 1.1;
    if (event->angleDelta().y() > 0) {
        scaleFactor *= zoomFactor; // Увеличиваем масштаб
    } else {
        scaleFactor /= zoomFactor; // Уменьшаем масштаб
    }
    scaleFactor = std::clamp(scaleFactor, 0.5, 3.0); // Ограничиваем масштаб

    update(); // Перерисовываем виджет
}

void StarMapWidget::saveImage() {
    QString filePath = QFileDialog::getSaveFileName(this, "Save Star Map", "", "PNG Images (*.png)");
    if (!filePath.isEmpty()) {
        if (!blurredImage.save(filePath)) {
            QMessageBox::warning(this, "Error", "Failed to save the image.");
        } else {
            QMessageBox::information(this, "Success", "Image saved successfully.");
        }
    }
}

void StarMapWidget::toggleInteractiveMode() {
    isInteractiveMode = !isInteractiveMode;
    interactiveModeButton->setText(isInteractiveMode ? "Disable Interactive Mode" : "Interactive Mode");
    update();
}

void StarMapWidget::resizeEvent(QResizeEvent* event) {
    // Обновляем позиции кнопок при изменении размера окна
    saveButton->move(width() - saveButton->width() - 10, 10);
    interactiveModeButton->move(10, 10);
    QWidget::resizeEvent(event);
}
