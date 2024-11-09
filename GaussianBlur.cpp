#include "GaussianBlur.h"
#include <cmath>
#include <algorithm>

// Преобразование изображения в оттенки серого
QImage GaussianBlur::toGrayscale(const QImage& image) {
    QImage grayscaleImage = image.convertToFormat(QImage::Format_Grayscale8);
    return grayscaleImage;
}

// Генерация ядра Гаусса
std::vector<std::vector<double>> GaussianBlur::createGaussianKernel(int kernelSize, double sigmaX, double sigmaY, double rho, double mX, double mY) {
    std::vector<std::vector<double>> kernel(kernelSize, std::vector<double>(kernelSize));
    int halfSize = kernelSize / 2;
    double sum = 0.0;

    // Заполняем ядро значениями функции Гаусса
    for (int y = -halfSize; y <= halfSize; y++) {
        for (int x = -halfSize; x <= halfSize; x++) {
            double value = gaussian(x, y, sigmaX, sigmaY, rho, mX, mY);
            kernel[y + halfSize][x + halfSize] = value;
            sum += value;
        }
    }

    // Нормализация ядра
    for (int y = 0; y < kernelSize; y++) {
        for (int x = 0; x < kernelSize; x++) {
            kernel[y][x] /= sum;
        }
    }

    return kernel;
}

// Вычисление значения двумерной функции Гаусса
double GaussianBlur::gaussian(int x, int y, double sigmaX, double sigmaY, double rho, double mX, double mY) {
    // Константа нормализации
    double C = 1.0 / (2 * M_PI * sigmaX * sigmaY * std::sqrt(1 - rho * rho));

    // Центральная часть выражения, зависящая от x, y, mX, mY, sigmaX, sigmaY, rho
    double dx = x - mX;
    double dy = y - mY;

    double exponent = -1.0 / (2 * (1 - rho * rho)) * (
                          (dx * dx) / (sigmaX * sigmaX) -
                          2 * rho * (dx * dy) / (sigmaX * sigmaY) +
                          (dy * dy) / (sigmaY * sigmaY)
                          );

    return C * std::exp(exponent);
}

// Применение фильтра Гаусса к изображению
QImage GaussianBlur::applyGaussianBlur(const QImage& image, int kernelSize, double sigmaX, double sigmaY, double rho, double mX, double mY) {
    QImage grayscaleImage = toGrayscale(image);  // Перевод в черно-белое изображение
    int width = grayscaleImage.width();
    int height = grayscaleImage.height();
    QImage resultImage(width, height, QImage::Format_Grayscale8);

    auto kernel = createGaussianKernel(kernelSize, sigmaX, sigmaY, rho, mX, mY);
    int halfSize = kernelSize / 2;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double sum = 0.0;

            // Применение свертки
            for (int ky = 0; ky < kernelSize; ky++) {
                for (int kx = 0; kx < kernelSize; kx++) {
                    int pixelX = std::clamp(x + kx - halfSize, 0, width - 1);
                    int pixelY = std::clamp(y + ky - halfSize, 0, height - 1);
                    int pixelValue = qGray(grayscaleImage.pixel(pixelX, pixelY));
                    sum += pixelValue * kernel[ky][kx];
                }
            }

            int newValue = std::clamp(static_cast<int>(sum), 0, 255);
            resultImage.setPixel(x, y, qRgb(newValue, newValue, newValue));
        }
    }

    return resultImage;
}
