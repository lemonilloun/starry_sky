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
    QImage src = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    int width = src.width();
    int height = src.height();
    QImage resultImage(width, height, QImage::Format_ARGB32_Premultiplied);

    auto kernel = createGaussianKernel(kernelSize, sigmaX, sigmaY, rho, mX, mY);
    int halfSize = kernelSize / 2;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double sumR = 0.0;
            double sumG = 0.0;
            double sumB = 0.0;
            double sumA = 0.0;

            // Применение свертки
            for (int ky = 0; ky < kernelSize; ky++) {
                for (int kx = 0; kx < kernelSize; kx++) {
                    int pixelX = std::clamp(x + kx - halfSize, 0, width - 1);
                    int pixelY = std::clamp(y + ky - halfSize, 0, height - 1);
                    QRgb pixelValue = src.pixel(pixelX, pixelY);
                    const double w = kernel[ky][kx];
                    sumR += qRed(pixelValue)   * w;
                    sumG += qGreen(pixelValue) * w;
                    sumB += qBlue(pixelValue)  * w;
                    sumA += qAlpha(pixelValue) * w;
                }
            }

            int r = std::clamp(static_cast<int>(sumR), 0, 255);
            int g = std::clamp(static_cast<int>(sumG), 0, 255);
            int b = std::clamp(static_cast<int>(sumB), 0, 255);
            int a = std::clamp(static_cast<int>(sumA), 0, 255);
            resultImage.setPixel(x, y, qRgba(r, g, b, a));
        }
    }

    return resultImage;
}
