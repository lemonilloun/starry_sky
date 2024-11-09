#ifndef GAUSSIANBLUR_H
#define GAUSSIANBLUR_H

#include <QImage>
#include <vector>

class GaussianBlur {
public:
    // Применение фильтра Гаусса к изображению
    static QImage applyGaussianBlur(const QImage& image, int kernelSize, double sigmaX, double sigmaY, double rho, double mX = 0, double mY = 0);

private:
    // Генерация ядра Гаусса
    static std::vector<std::vector<double>> createGaussianKernel(int kernelSize, double sigmaX, double sigmaY, double rho, double mX, double mY);

    // Вычисление значения двумерной функции Гаусса
    static double gaussian(int x, int y, double sigmaX, double sigmaY, double rho, double mX, double mY);

    // Преобразование изображения в оттенки серого
    static QImage toGrayscale(const QImage& image);
};

#endif // GAUSSIANBLUR_H
