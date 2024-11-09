#ifndef GAUSSIANBLUR_H
#define GAUSSIANBLUR_H

#include <QImage>
#include <vector>

class GaussianBlur {
public:
    // Применение фильтра Гаусса к изображению
    static QImage applyGaussianBlur(const QImage& image, int kernelSize, double sigma);

private:
    // Генерация ядра Гаусса
    static std::vector<std::vector<double>> createGaussianKernel(int kernelSize, double sigma);

    // Вычисление значения Гаусса
    static double gaussian(int x, int y, double sigma);

    // Преобразование изображения в оттенки серого
    static QImage toGrayscale(const QImage& image);
};

#endif // GAUSSIANBLUR_H
