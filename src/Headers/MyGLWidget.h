#ifndef MYGLWIDGET_H
#define MYGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QVector3D>
#include <QVector2D>
#include <QString>
#include <vector>

class MyGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit MyGLWidget(QWidget* parent = nullptr);
    ~MyGLWidget() override;

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

    bool loadObjModel(const QString& path);
    bool loadTexture(const QString& path);
    bool loadBackgroundTexture(const QString& path);
    QString resolveModelPath() const;
    QString resolveTexturePath(const QString& objPath) const;
    QString resolveBackgroundPath() const;
    void drawModel();
    void drawFallbackModel();
    void drawBackground();

    std::vector<QVector3D> m_modelVertices;
    std::vector<QVector3D> m_modelNormals;
    std::vector<QVector2D> m_modelTexCoords;
    QVector3D              m_modelCenter {0.0f, 0.0f, 0.0f};
    float                  m_modelScale = 1.0f;
    bool                   m_modelLoaded = false;
    bool                   m_modelTextured = false;
    GLuint                 m_textureId = 0;
    GLuint                 m_backgroundTextureId = 0;
    bool                   m_backgroundLoaded = false;
};

#endif // MYGLWIDGET_H
