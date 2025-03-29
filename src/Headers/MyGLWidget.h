#ifndef MYGLWIDGET_H
#define MYGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

class MyGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit MyGLWidget(QWidget* parent = nullptr);

    // Методы, чтобы задать углы вращения (в градусах)
    void setAngles(double thetaDeg, double psiDeg, double phiDeg);

protected:
    // Эти три метода перекрываются для рисования
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    double m_theta = 0.0;
    double m_psi   = 0.0;
    double m_phi   = 0.0;

    // Вспомогательный метод для рисования «двух перекрещенных прямоугольников»
    void drawSensorModel();
};

#endif // MYGLWIDGET_H
