#include "MyGLWidget.h"
#include <QDebug>
#include <QMatrix4x4>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDir>
#include <QCoreApplication>
#include <QStringList>
#include <QImage>
#include <algorithm>
#include <limits>

MyGLWidget::MyGLWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    // Можно включить "сглаживание" или что-то ещё, если нужно
    // setFormat(QSurfaceFormat::defaultFormat());
}

MyGLWidget::~MyGLWidget()
{
    makeCurrent();
    if (m_textureId != 0) {
        glDeleteTextures(1, &m_textureId);
        m_textureId = 0;
    }
    doneCurrent();
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

    glClearColor(0.58f, 0.62f, 0.78f, 1.0f);

    // Включаем тест глубины (чтобы 3D-объекты рисовались правильно)
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_NORMALIZE);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    const GLfloat ambientScene[]  = {0.55f, 0.6f, 0.68f, 1.0f};
    const GLfloat ambientLight[]  = {0.65f, 0.65f, 0.7f, 1.0f};
    const GLfloat diffuseLight[]  = {0.95f, 0.95f, 0.95f, 1.0f};
    const GLfloat specularLight[] = {0.85f, 0.85f, 0.85f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientScene);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diffuseLight);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);

    const QString modelPath = resolveModelPath();
    if (!modelPath.isEmpty()) {
        m_modelLoaded = loadObjModel(modelPath);
        if (m_modelLoaded) {
            const QString texturePath = resolveTexturePath(modelPath);
            if (!texturePath.isEmpty())
                m_modelTextured = loadTexture(texturePath);
            qDebug() << "Loaded OBJ model from" << modelPath
                     << "texture" << (m_modelTextured ? texturePath : QStringLiteral("<none>"));
        } else {
            qWarning() << "Failed to load OBJ model, fallback to primitive geometry";
        }
    } else {
        qWarning() << "OBJ model path not found, fallback to primitive geometry";
    }

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

    if (m_modelLoaded) {
        glEnable(GL_LIGHTING);
        GLfloat lightPos[] = { 4.0f, 4.0f, 6.0f, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
        drawModel();
        glDisable(GL_LIGHTING);
    } else {
        drawFallbackModel();
    }
}

void MyGLWidget::drawModel()
{
    if (m_modelVertices.empty())
        return;

    glPushMatrix();
    glScalef(m_modelScale, m_modelScale, m_modelScale);
    glTranslatef(-m_modelCenter.x(), -m_modelCenter.y(), -m_modelCenter.z());

    const bool hasNormals = m_modelNormals.size() == m_modelVertices.size();
    const bool hasTex = m_modelTextured && m_modelTexCoords.size() == m_modelVertices.size();

    if (hasTex) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, m_textureId);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glColor3f(1.0f, 1.0f, 1.0f);
    } else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.75f, 0.78f, 0.82f);
    }


    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < m_modelVertices.size(); ++i) {
        if (hasNormals) {
            const QVector3D& n = m_modelNormals[i];
            glNormal3f(n.x(), n.y(), n.z());
        }
        if (hasTex) {
            const QVector2D& uv = m_modelTexCoords[i];
            glTexCoord2f(uv.x(), uv.y());
        }
        const QVector3D& v = m_modelVertices[i];
        glVertex3f(v.x(), v.y(), v.z());
    }
    glEnd();

    if (hasTex) {
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    }

    glPopMatrix();
}

void MyGLWidget::drawFallbackModel()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

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

QString MyGLWidget::resolveModelPath() const
{
    const QString relativePath = QStringLiteral("src/data/model/10477_Satellite_v1_L3.obj");
    QStringList roots = {
        QCoreApplication::applicationDirPath(),
        QDir::currentPath()
    };

    for (const QString& root : roots) {
        QDir dir(root);
        for (int depth = 0; depth < 6; ++depth) {
            const QString candidate = dir.filePath(relativePath);
            if (QFile::exists(candidate)) {
                return QFileInfo(candidate).canonicalFilePath();
            }
            if (!dir.cdUp())
                break;
        }
    }

    // Проверяем абсолютный путь от корня репозитория, если приложение запускается оттуда.
    const QString directPath = QDir::cleanPath(QStringLiteral("/Users/lehacho/starry_sky/") + relativePath);
    if (QFile::exists(directPath))
        return QFileInfo(directPath).canonicalFilePath();

    return {};
}

QString MyGLWidget::resolveTexturePath(const QString& objPath) const
{
    const QFileInfo objInfo(objPath);
    const QDir baseDir = objInfo.dir();

    const QStringList candidates = {
        QStringLiteral("10477_Satellite_v1_Diffuse.jpg"),
        QStringLiteral("10477_Satellite_v1_L3.jpg"),
        QStringLiteral("10477_Satellite_v1_Diffuse.png")
    };

    for (const QString& name : candidates) {
        const QString candidate = baseDir.filePath(name);
        if (QFile::exists(candidate))
            return QFileInfo(candidate).canonicalFilePath();
    }
    return {};
}

bool MyGLWidget::loadObjModel(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open OBJ file" << path;
        return false;
    }

    QTextStream in(&file);
    std::vector<QVector3D> positions;
    std::vector<QVector3D> normals;
    std::vector<QVector2D> texcoords;
    std::vector<QVector3D> finalVertices;
    std::vector<QVector3D> finalNormals;
    std::vector<QVector2D> finalTexCoords;

    positions.reserve(50000);
    normals.reserve(50000);
    finalVertices.reserve(150000);
    finalNormals.reserve(150000);

    struct FaceVertex {
        int v = -1;
        int t = -1;
        int n = -1;
    };

    auto resolveIndex = [](int rawIndex, int count) -> int {
        if (count == 0)
            return -1;
        if (rawIndex > 0) {
            rawIndex -= 1;
        } else if (rawIndex < 0) {
            rawIndex = count + rawIndex;
        } else {
            return -1;
        }
        if (rawIndex < 0 || rawIndex >= count)
            return -1;
        return rawIndex;
    };

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        line.replace('\t', ' ');
        const QStringList tokens = line.split(' ', Qt::SkipEmptyParts);
        if (tokens.isEmpty())
            continue;

        const QString& tag = tokens[0];
        if (tag == QLatin1String("v") && tokens.size() >= 4) {
            bool okX = false, okY = false, okZ = false;
            float x = tokens[1].toFloat(&okX);
            float y = tokens[2].toFloat(&okY);
            float z = tokens[3].toFloat(&okZ);
            if (okX && okY && okZ) {
                positions.emplace_back(x, y, z);
            }
        } else if (tag == QLatin1String("vt") && tokens.size() >= 3) {
            bool okU = false, okV = false;
            float u = tokens[1].toFloat(&okU);
            float v = tokens[2].toFloat(&okV);
            if (okU && okV) {
                texcoords.emplace_back(u, v);
            }
        } else if (tag == QLatin1String("vn") && tokens.size() >= 4) {
            bool okX = false, okY = false, okZ = false;
            float x = tokens[1].toFloat(&okX);
            float y = tokens[2].toFloat(&okY);
            float z = tokens[3].toFloat(&okZ);
            if (okX && okY && okZ) {
                QVector3D n(x, y, z);
                if (!n.isNull())
                    n.normalize();
                normals.push_back(n);
            }
        } else if (tag == QLatin1String("f") && tokens.size() >= 4) {
            std::vector<FaceVertex> face;
            face.reserve(tokens.size() - 1);

            for (int i = 1; i < tokens.size(); ++i) {
                const QString& vertToken = tokens[i];
                const QStringList parts = vertToken.split('/', Qt::KeepEmptyParts);
                if (parts.isEmpty())
                    continue;

                bool okV = false;
                int rawV = parts[0].toInt(&okV);
                if (!okV)
                    continue;

                FaceVertex fv;
                fv.v = resolveIndex(rawV, static_cast<int>(positions.size()));
                if (fv.v < 0)
                    continue;

                if (parts.size() >= 2 && !parts[1].isEmpty()) {
                    bool okT = false;
                    int rawT = parts[1].toInt(&okT);
                    if (okT) {
                        fv.t = resolveIndex(rawT, static_cast<int>(texcoords.size()));
                    }
                }

                if (parts.size() >= 3 && !parts[2].isEmpty()) {
                    bool okN = false;
                    int rawN = parts[2].toInt(&okN);
                    if (okN) {
                        fv.n = resolveIndex(rawN, static_cast<int>(normals.size()));
                    }
                }

                face.push_back(fv);
            }

            if (face.size() < 3)
                continue;

            for (size_t i = 1; i + 1 < face.size(); ++i) {
                const FaceVertex& f0 = face[0];
                const FaceVertex& f1 = face[i];
                const FaceVertex& f2 = face[i + 1];

                if (f0.v < 0 || f1.v < 0 || f2.v < 0)
                    continue;

                const QVector3D& p0 = positions[static_cast<size_t>(f0.v)];
                const QVector3D& p1 = positions[static_cast<size_t>(f1.v)];
                const QVector3D& p2 = positions[static_cast<size_t>(f2.v)];

                QVector3D computedNormal = QVector3D::crossProduct(p1 - p0, p2 - p0);
                if (!computedNormal.isNull())
                    computedNormal.normalize();
                else
                    computedNormal = QVector3D(0.0f, 0.0f, 1.0f);

                const bool useFileNormals =
                    f0.n >= 0 && f1.n >= 0 && f2.n >= 0 &&
                    f0.n < static_cast<int>(normals.size()) &&
                    f1.n < static_cast<int>(normals.size()) &&
                    f2.n < static_cast<int>(normals.size());

                const bool useFileTex =
                    f0.t >= 0 && f1.t >= 0 && f2.t >= 0 &&
                    f0.t < static_cast<int>(texcoords.size()) &&
                    f1.t < static_cast<int>(texcoords.size()) &&
                    f2.t < static_cast<int>(texcoords.size());

                finalVertices.push_back(p0);
                finalVertices.push_back(p1);
                finalVertices.push_back(p2);

                if (useFileNormals) {
                    finalNormals.push_back(normals[static_cast<size_t>(f0.n)]);
                    finalNormals.push_back(normals[static_cast<size_t>(f1.n)]);
                    finalNormals.push_back(normals[static_cast<size_t>(f2.n)]);
                } else {
                    finalNormals.push_back(computedNormal);
                    finalNormals.push_back(computedNormal);
                    finalNormals.push_back(computedNormal);
                }

                if (useFileTex) {
                    const QVector2D& uv0 = texcoords[static_cast<size_t>(f0.t)];
                    const QVector2D& uv1 = texcoords[static_cast<size_t>(f1.t)];
                    const QVector2D& uv2 = texcoords[static_cast<size_t>(f2.t)];
                    finalTexCoords.emplace_back(uv0.x(), 1.0f - uv0.y());
                    finalTexCoords.emplace_back(uv1.x(), 1.0f - uv1.y());
                    finalTexCoords.emplace_back(uv2.x(), 1.0f - uv2.y());
                } else {
                    finalTexCoords.emplace_back(0.0f, 0.0f);
                    finalTexCoords.emplace_back(0.0f, 0.0f);
                    finalTexCoords.emplace_back(0.0f, 0.0f);
                }
            }
        }
    }

    if (finalVertices.empty()) {
        qWarning() << "OBJ loader: no triangles were parsed from" << path;
        return false;
    }

    QVector3D minV(std::numeric_limits<float>::max(),
                   std::numeric_limits<float>::max(),
                   std::numeric_limits<float>::max());
    QVector3D maxV(-std::numeric_limits<float>::max(),
                   -std::numeric_limits<float>::max(),
                   -std::numeric_limits<float>::max());

    for (const QVector3D& v : finalVertices) {
        minV.setX(std::min(minV.x(), v.x()));
        minV.setY(std::min(minV.y(), v.y()));
        minV.setZ(std::min(minV.z(), v.z()));

        maxV.setX(std::max(maxV.x(), v.x()));
        maxV.setY(std::max(maxV.y(), v.y()));
        maxV.setZ(std::max(maxV.z(), v.z()));
    }

    QVector3D extent = maxV - minV;
    float maxExtent = std::max({extent.x(), extent.y(), extent.z()});
    if (maxExtent <= 0.0f)
        maxExtent = 1.0f;

    m_modelCenter = (minV + maxV) * 0.5f;
    m_modelScale  = 6.8f / maxExtent;

    m_modelVertices = std::move(finalVertices);
    m_modelNormals  = std::move(finalNormals);
    m_modelTexCoords = std::move(finalTexCoords);

    qDebug() << "OBJ loader: triangles =" << m_modelVertices.size() / 3
             << "scale =" << m_modelScale;

    return true;
}

bool MyGLWidget::loadTexture(const QString& path)
{
    QImage image(path);
    if (image.isNull()) {
        qWarning() << "Failed to load texture image" << path;
        return false;
    }

    image = image.convertToFormat(QImage::Format_RGBA8888);

    if (m_textureId == 0) {
        glGenTextures(1, &m_textureId);
    }

    glBindTexture(GL_TEXTURE_2D, m_textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 image.width(),
                 image.height(),
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 image.constBits());
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}
