#include "MyGLWidget.h"
#include <QDebug>
#include <QMatrix4x4>
#include <QPainter>

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

   glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

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

    // Матрица вида:
    QMatrix4x4 view;
    // Сдвигаем камеру "справа и сверху"
    view.lookAt(QVector3D(3.0f, 2.0f, 5.0f),
                QVector3D(0.0f, 0.0f, 0.0f),
                QVector3D(0.0f, 1.0f, 0.0f));
    glMultMatrixf(view.constData());

    // повороты Rx(theta), Ry(psi), Rz(phi)
    glRotatef(float(m_phi),   0.f, 0.f, 1.f);
    glRotatef(float(m_psi),   0.f, 1.f, 0.f);
    glRotatef(float(m_theta), 1.f, 0.f, 0.f);

    // Рисуем утолщённые оси
    glLineWidth(10.0f);

    glBegin(GL_LINES);
    // X — красная
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0,0,0); glVertex3f(3,0,0);

    // Y — зелёная
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0,0,0); glVertex3f(0,2,0);

    // Z — синяя
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0,0,0); glVertex3f(0,0,2.5);
    glEnd();

    // Рисуем две плоскости-«прямоугольника»
    drawSensorModel();

    // Теперь выводим 2D-текст (подпись) с помощью QPainter
    QPainter painter(this);
    painter.setPen(Qt::white);
    painter.end();
}

void MyGLWidget::drawSensorModel()
{
    // размеры «тела» сенсора (синяя часть)
    const float bodyHalfThickness = 0.5f;  // толщина по X
    const float bodyHalfHeight    = 0.5f;  // по Y
    const float bodyHalfDepth     = 0.7f;  // по Z

    // рисуем синий «бокс» – 6 граней кубоида
    glColor3f(0.0f, 0.8f, 0.8f); // бирюзовый
    glBegin(GL_QUADS);
    // передняя грань (X = +bodyHalfThickness)
    glVertex3f( bodyHalfThickness, -bodyHalfHeight, -bodyHalfDepth);
    glVertex3f( bodyHalfThickness,  bodyHalfHeight, -bodyHalfDepth);
    glVertex3f( bodyHalfThickness,  bodyHalfHeight,  bodyHalfDepth);
    glVertex3f( bodyHalfThickness, -bodyHalfHeight,  bodyHalfDepth);
    // задняя грань (X = -bodyHalfThickness)
    glVertex3f(-bodyHalfThickness, -bodyHalfHeight,  bodyHalfDepth);
    glVertex3f(-bodyHalfThickness,  bodyHalfHeight,  bodyHalfDepth);
    glVertex3f(-bodyHalfThickness,  bodyHalfHeight, -bodyHalfDepth);
    glVertex3f(-bodyHalfThickness, -bodyHalfHeight, -bodyHalfDepth);
    // верхняя грань (Y = +bodyHalfHeight)
    glVertex3f(-bodyHalfThickness,  bodyHalfHeight, -bodyHalfDepth);
    glVertex3f( bodyHalfThickness,  bodyHalfHeight, -bodyHalfDepth);
    glVertex3f( bodyHalfThickness,  bodyHalfHeight,  bodyHalfDepth);
    glVertex3f(-bodyHalfThickness,  bodyHalfHeight,  bodyHalfDepth);
    // нижняя грань (Y = -bodyHalfHeight)
    glVertex3f(-bodyHalfThickness, -bodyHalfHeight,  bodyHalfDepth);
    glVertex3f( bodyHalfThickness, -bodyHalfHeight,  bodyHalfDepth);
    glVertex3f( bodyHalfThickness, -bodyHalfHeight, -bodyHalfDepth);
    glVertex3f(-bodyHalfThickness, -bodyHalfHeight, -bodyHalfDepth);
    // правая грань (Z = +bodyHalfDepth)
    glVertex3f(-bodyHalfThickness, -bodyHalfHeight,  bodyHalfDepth);
    glVertex3f(-bodyHalfThickness,  bodyHalfHeight,  bodyHalfDepth);
    glVertex3f( bodyHalfThickness,  bodyHalfHeight,  bodyHalfDepth);
    glVertex3f( bodyHalfThickness, -bodyHalfHeight,  bodyHalfDepth);
    // левая грань (Z = -bodyHalfDepth)
    glVertex3f( bodyHalfThickness, -bodyHalfHeight, -bodyHalfDepth);
    glVertex3f( bodyHalfThickness,  bodyHalfHeight, -bodyHalfDepth);
    glVertex3f(-bodyHalfThickness,  bodyHalfHeight, -bodyHalfDepth);
    glVertex3f(-bodyHalfThickness, -bodyHalfHeight, -bodyHalfDepth);
    glEnd();

    // толщину выдавливания задаём единожды
    const float panelDepth = 0.3f;
    const float halfDepth  = panelDepth * 0.5f;

    // === жёлтые боковые панели ===
    {
        const float extraLen       = 0.3f;                   // на сколько выступает
        const float panelHalfLen   = 0.5f + extraLen;        // полуширина по X
        const float panelHalfHei   = 0.1f;                   // полуполовина по Y

        glColor3f(0.9f, 0.9f, 0.0f);
        for (int side : {-1, +1}) {
            // смещение центра панели вдоль X
            float cx = side * (bodyHalfThickness + panelHalfLen);
            // координаты по X,Y
            float x0 = cx - panelHalfLen, x1 = cx + panelHalfLen;
            float y0 = -panelHalfHei,     y1 = +panelHalfHei;
            // Z-координаты для «передней» и «задней» граней
            float z0 = -halfDepth,        z1 = +halfDepth;

            // 6 граней тонкого параллелепипеда:
            glBegin(GL_QUADS);
            // передняя грань (Z = +halfDepth)
            glVertex3f(x0, y0, z1);
            glVertex3f(x1, y0, z1);
            glVertex3f(x1, y1, z1);
            glVertex3f(x0, y1, z1);
            // задняя грань (Z = -halfDepth)
            glVertex3f(x1, y0, z0);
            glVertex3f(x0, y0, z0);
            glVertex3f(x0, y1, z0);
            glVertex3f(x1, y1, z0);
            // правая грань
            glVertex3f(x1, y0, z1);
            glVertex3f(x1, y0, z0);
            glVertex3f(x1, y1, z0);
            glVertex3f(x1, y1, z1);
            // левая грань
            glVertex3f(x0, y0, z0);
            glVertex3f(x0, y0, z1);
            glVertex3f(x0, y1, z1);
            glVertex3f(x0, y1, z0);
            // верхняя грань
            glVertex3f(x0, y1, z1);
            glVertex3f(x1, y1, z1);
            glVertex3f(x1, y1, z0);
            glVertex3f(x0, y1, z0);
            // нижняя грань
            glVertex3f(x0, y0, z0);
            glVertex3f(x1, y0, z0);
            glVertex3f(x1, y0, z1);
            glVertex3f(x0, y0, z1);
            glEnd();
        }
    }

    // === фиолетовые доп-панели ===
    {
        const float violetLen    = 1.0f;   // длина по X
        const float violetHei    = 1.5f;   // высота по Y
        const float halfLen      = violetLen * 0.5f;
        const float halfHei      = violetHei * 0.5f;

        glColor3f(0.6f, 0.0f, 0.6f);
        for (int side : {-1, +1}) {
            float cx = side * (bodyHalfThickness + /*жёлтая полупанель*/ (0.5f + 0.3f) + halfLen);
            float x0 = cx - halfLen, x1 = cx + halfLen;
            float y0 = -halfHei,     y1 = +halfHei;
            float z0 = -halfDepth,   z1 = +halfDepth;

            glBegin(GL_QUADS);
            // передняя
            glVertex3f(x0, y0, z1);
            glVertex3f(x1, y0, z1);
            glVertex3f(x1, y1, z1);
            glVertex3f(x0, y1, z1);
            // задняя
            glVertex3f(x1, y0, z0);
            glVertex3f(x0, y0, z0);
            glVertex3f(x0, y1, z0);
            glVertex3f(x1, y1, z0);
            // правая
            glVertex3f(x1, y0, z1);
            glVertex3f(x1, y0, z0);
            glVertex3f(x1, y1, z0);
            glVertex3f(x1, y1, z1);
            // левая
            glVertex3f(x0, y0, z0);
            glVertex3f(x0, y0, z1);
            glVertex3f(x0, y1, z1);
            glVertex3f(x0, y1, z0);
            // верхняя
            glVertex3f(x0, y1, z1);
            glVertex3f(x1, y1, z1);
            glVertex3f(x1, y1, z0);
            glVertex3f(x0, y1, z0);
            // нижняя
            glVertex3f(x0, y0, z0);
            glVertex3f(x1, y0, z0);
            glVertex3f(x1, y0, z1);
            glVertex3f(x0, y0, z1);
            glEnd();
        }
    }
}
