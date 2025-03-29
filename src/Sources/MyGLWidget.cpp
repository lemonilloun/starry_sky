#include "MyGLWidget.h"
#include <QDebug>
#include <QMatrix4x4>

MyGLWidget::MyGLWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    // Можно включить "сглаживание" или что-то ещё, если нужно
    // setFormat(QSurfaceFormat::defaultFormat());
}

void MyGLWidget::setAngles(double thetaDeg, double psiDeg, double phiDeg)
{
    m_theta = thetaDeg;
    m_psi   = psiDeg;
    m_phi   = phiDeg;

    // Просим OpenGL-подсистему перерисоваться
    update();
}

void MyGLWidget::initializeGL()
{
    // Инициализация OpenGL-функций
    initializeOpenGLFunctions();

    // Устанавливаем цвет фона (глубокий серый, скажем)
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    // Включаем тест глубины (чтобы 3D-объекты рисовались правильно)
    glEnable(GL_DEPTH_TEST);

    qDebug() << "MyGLWidget initialized!";
}

void MyGLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Создаём матрицу перспективы
    QMatrix4x4 projection;
    float aspect = float(w) / float(h ? h : 1);
    projection.perspective(45.0f, aspect, 0.1f, 100.0f);

    // Умножаем «текущую» (единичную) матрицу OpenGL на нашу
    glMultMatrixf(projection.constData());

    // Переходим обратно в модельно-видовую
    glMatrixMode(GL_MODELVIEW);
}

void MyGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Вместо gluLookAt — QMatrix4x4::lookAt
    QMatrix4x4 view;
    view.lookAt(
        QVector3D(0.f, 0.f, 5.f),  // Позиция "камеры"
        QVector3D(0.f, 0.f, 0.f),  // Точка, куда смотрим
        QVector3D(0.f, 1.f, 0.f)   // Вектор "вверх"
        );
    glMultMatrixf(view.constData());

    // Теперь можно применять ваши повороты:
    glRotatef(float(m_phi),   0.f, 0.f, 1.f);
    glRotatef(float(m_psi),   0.f, 1.f, 0.f);
    glRotatef(float(m_theta), 1.f, 0.f, 0.f);

    // Нарисуем «ось X, Y, Z» для наглядности
    //  -- обычно рисуют линиями
    glBegin(GL_LINES);
    // Ось X (красная)
    glColor3f(1,0,0);
    glVertex3f(0,0,0); glVertex3f(2,0,0);
    // Ось Y (зелёная)
    glColor3f(0,1,0);
    glVertex3f(0,0,0); glVertex3f(0,2,0);
    // Ось Z (синяя)
    glColor3f(0,0,1);
    glVertex3f(0,0,0); glVertex3f(0,0,2);
    glEnd();

    // Теперь рисуем наш «датчик» (два прямоугольника)
    drawSensorModel();
}

void MyGLWidget::drawSensorModel()
{
    // Примитивный способ: нарисуем 2 плоскости (квадрата),
    // перпендикулярных друг другу: один в плоскости XZ, другой в плоскости YZ.
    // И оба пересекаются по оси Z (к примеру).

    // Первый «прямоугольник» (квадрат) в плоскости XZ, центр в (0,0,0), размер 1x1
    glColor3f(0.9f, 0.9f, 0.0f); // желтоватый
    glBegin(GL_QUADS);
    glVertex3f(-0.5f, 0.0f, -0.5f);
    glVertex3f( 0.5f, 0.0f, -0.5f);
    glVertex3f( 0.5f, 0.0f,  0.5f);
    glVertex3f(-0.5f, 0.0f,  0.5f);
    glEnd();

    // Второй «прямоугольник» (квадрат) в плоскости YZ, пересекающийся по оси Z
    glColor3f(0.0f, 0.8f, 0.8f); // бирюзоватый
    glBegin(GL_QUADS);
    glVertex3f(0.0f, -0.5f, -0.5f);
    glVertex3f(0.0f,  0.5f, -0.5f);
    glVertex3f(0.0f,  0.5f,  0.5f);
    glVertex3f(0.0f, -0.5f,  0.5f);
    glEnd();
}
