#include <QImage>
#include <cmath>
#include <vector>

class GaussianBlur {
public:
    // Применение фильтра Гаусса к изображению
    static QImage applyGaussianBlur(const QImage& image, int kernelSize, double sigma);

private:
    static std::vector<std::vector<double>> createGaussianKernel(int kernelSize, double sigma);
    static double gaussian(int x, int y, double sigma);
    static QImage toGrayscale(const QImage& image);
};

// Преобразование изображения в оттенки серого
QImage GaussianBlur::toGrayscale(const QImage& image) {
    QImage grayscaleImage = image.convertToFormat(QImage::Format_Grayscale8);
    return grayscaleImage;
}

// Генерация ядра Гаусса
std::vector<std::vector<double>> GaussianBlur::createGaussianKernel(int kernelSize, double sigma) {
    std::vector<std::vector<double>> kernel(kernelSize, std::vector<double>(kernelSize));
    int halfSize = kernelSize / 2;
    double sum = 0.0;

    // Заполняем ядро значениями Гаусса
    for (int y = -halfSize; y <= halfSize; y++) {
        for (int x = -halfSize; x <= halfSize; x++) {
            double value = gaussian(x, y, sigma);
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

// Вычисление значения Гаусса
double GaussianBlur::gaussian(int x, int y, double sigma) {
    return (1 / (2 * M_PI * sigma * sigma)) * std::exp(-(x * x + y * y) / (2 * sigma * sigma));
}

// Применение свертки с ядром Гаусса
QImage GaussianBlur::applyGaussianBlur(const QImage& image, int kernelSize, double sigma) {
    QImage grayscaleImage = toGrayscale(image);  // Перевод в черно-белое изображение
    int width = grayscaleImage.width();
    int height = grayscaleImage.height();
    QImage resultImage(width, height, QImage::Format_Grayscale8);

    auto kernel = createGaussianKernel(kernelSize, sigma);
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
