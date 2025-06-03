#include "include/shapeViewer.h"
#include "remap.h"
#include "transformation.h"
#include <QPainter>
#include <QMouseEvent>
#include <QRandomGenerator>

ShapeViewer::ShapeViewer(QWidget *parent)
    : QWidget(parent)
{
    baseRadius = 50;
    cylinderHeight = 100;
    subDivisons = 20;

    QVector3D cameraPos = QVector3D(300.0f, 200.0f, 50.0f);
    QVector3D lookAt = QVector3D(0.0f, cylinderHeight / 2, 0.0f);
    QVector3D upVec = QVector3D(0.0f, 1.0f, 0.0f);

    camera = Camera(cameraPos, lookAt, upVec);

    buildGeometry();
}

void ShapeViewer::paintEvent(QPaintEvent *)
{
    // Create framebuffer
    QImage framebuffer(width(), height(), QImage::Format_RGB32);
    framebuffer.fill(Qt::black);

    QMatrix4x4 view = getViewMatrix(camera.getPosition(), camera.getLookAt(), camera.getUpVector());
    QMatrix4x4 proj = getProjectionMatrix(width(), height(), 45.0f);
    QMatrix4x4 vp = proj * view;

    auto project = [&](const QVector4D &v)
    {
        QVector4D p = vp * v;
        if (p.w() != 0)
            p /= p.w();
        return QPointF(p.x(), p.y());
    };

    // Precompute projected vertices
    QVector<QPointF> projected;
    for (const auto &v : vertices)
        projected.append(project(v.position));

    QVector3D cam = camera.getPosition();

    for (int i = 0; i < triangles.size(); ++i)
    {
        const auto &tri = triangles[i].tri;
        QVector3D v1 = vertices[int(tri.x())].position.toVector3D();
        QVector3D v2 = vertices[int(tri.y())].position.toVector3D();
        QVector3D v3 = vertices[int(tri.z())].position.toVector3D();

        float d1 = (v1 - cam).length();
        float d2 = (v2 - cam).length();
        float d3 = (v3 - cam).length();

        triangles[i].avgDistance = (d1 + d2 + d3) / 3.0f;
    }

    // Sort by depth (back to front)
    std::sort(triangles.begin(), triangles.end(),
              [](const Triangle &a, const Triangle &b)
              { return a.avgDistance > b.avgDistance; });

    for (const auto &tw : triangles)
    {
        int i1 = int(tw.tri.x()), i2 = int(tw.tri.y()), i3 = int(tw.tri.z());

        QVector3D p1 = vertices[i1].position.toVector3D();
        QVector3D p2 = vertices[i2].position.toVector3D();
        QVector3D p3 = vertices[i3].position.toVector3D();
        QVector3D normal = QVector3D::normal(p1, p2, p3);

        if (displayMode == DisplayMode::Wireframe)
        {
            float d = std::clamp(tw.avgDistance / 1000.0f, 0.0f, 1.0f);
            int intensity = int(std::pow(1.0f - d, 0.6f) * 255);
            QColor wireColor(intensity, intensity, intensity);

            QPainter meshPainter(&framebuffer);
            meshPainter.setPen(wireColor);
            meshPainter.drawLine(projected[i1], projected[i2]);
            meshPainter.drawLine(projected[i2], projected[i3]);
            meshPainter.drawLine(projected[i3], projected[i1]);
        }
        else if (displayMode == DisplayMode::Textured && !textureImage.isNull())
        {
            if (!isFacingFront(normal, cam, camera.getLookAt()))
                continue;
            remapTriangleToImage(framebuffer,
                                 projected[i1], projected[i2], projected[i3],
                                 vertices[i1].textureCoord, vertices[i2].textureCoord, vertices[i3].textureCoord,
                                 textureImage, Qt::black);
        }
        else
        {
            if (!isFacingFront(normal, cam, camera.getLookAt()))
                continue;
            remapTriangleToImage(framebuffer,
                                 projected[i1], projected[i2], projected[i3],
                                 QVector2D(), QVector2D(), QVector2D(),
                                 QImage(), tw.color);
        }
    }

    // Draw final framebuffer
    QPainter painter(this);
    painter.drawImage(0, 0, framebuffer);
}

bool ShapeViewer::isFacingFront(const QVector3D &normal, const QVector3D &cameraPos, const QVector3D &lookAt)
{
    QVector3D viewDir = (lookAt - cameraPos).normalized();
    return QVector3D::dotProduct(normal, viewDir) <= 0;
}

void ShapeViewer::mousePressEvent(QMouseEvent *event)
{
    lastMousePos = event->pos();
}

void ShapeViewer::mouseMoveEvent(QMouseEvent *event)
{
    QPoint delta = event->pos() - lastMousePos;
    lastMousePos = event->pos();

    camera.updatePosition(delta);

    update();
}

void ShapeViewer::wheelEvent(QWheelEvent *event)
{
    float delta = event->angleDelta().y() / 120.0f;

    camera.zoom(delta);

    update();
}

void ShapeViewer::setBaseRadius(int r)
{
    baseRadius = r;
    buildGeometry();
    update();
}

void ShapeViewer::setHeight(int h)
{
    cylinderHeight = h;
    buildGeometry();
    camera.setLookAt(QVector3D(0.0f, h / 2.0f, 0.0f));
    update();
}

void ShapeViewer::setSubdivisions(int s)
{
    subDivisons = s;
    buildGeometry();
    update();
}

void ShapeViewer::buildGeometry()
{
    int n = subDivisons;
    float r = baseRadius;
    float h = cylinderHeight;
    vertices = QVector<Vertex>(4 * n + 2);
    triangles = QVector<Triangle>(4 * n);

    // === Top ===
    vertices[0] = Vertex{QVector4D(0, h, 0, 1), QVector4D(0, 1, 0, 0), QVector2D(0.25f, 0.25f)};

    for (int i = 1; i < n + 1; ++i)
    {
        float angle = 2 * M_PI * (i - 1) / n;
        float x = r * cos(angle);
        float z = r * sin(angle);

        float texX = 0.25f * (1 + cos(angle));
        float texY = 0.25f * (1 + sin(angle));
        QVector2D tex = QVector2D(texX, texY);
        vertices[i] = Vertex{QVector4D(x, h, z, 1), QVector4D(0, 1, 0, 0), tex};
    }

    for (int i = 0; i < n - 1; i++)
    {
        triangles[i].tri = QVector3D(0, i + 2, i + 1);
    }

    triangles[n - 1].tri = QVector3D(0, 1, n);

    // === Bottom ===
    vertices[4 * n + 1] = Vertex{QVector4D(0, 0, 0, 1), QVector4D(0, -1, 0, 0), QVector2D(0.75f, 0.25f)};

    for (int i = 0; i < n; ++i)
    {
        float angle = 2 * M_PI * i / n;
        float x = r * cos(angle);
        float z = r * sin(angle);
        float texX = 0.25f * (3 + cos(angle));
        float texY = 0.25f * (1 + sin(angle));
        vertices[3 * n + i + 1] = Vertex{QVector4D(x, 0, z, 1), QVector4D(0, -1, 0, 0), QVector2D(texX, texY)};
    }

    for (int i = 3 * n; i < 4 * n - 1; i++)
    {
        triangles[i].tri = QVector3D(4 * n + 1, i + 1, i + 2);
    }

    triangles[4 * n - 1].tri = QVector3D(4 * n + 1, 4 * n, 3 * n + 1);

    // === Side ===
    for (int i = 0; i < n; ++i)
    {
        QVector4D pi = vertices[i + 1].position;
        QVector4D normal = QVector4D(pi.x() / r, 0, pi.z() / r, 0);
        float texX = float(i) / (n - 1);
        vertices[n + 1 + i] = Vertex{pi, normal, QVector2D(texX, 1.0f)};
    }

    for (int i = 0; i < n; ++i)
    {
        QVector4D pi = vertices[3 * n + 1 + i].position;
        QVector4D normal = QVector4D(pi.x() / r, 0, pi.z() / r, 0);
        float texX = float(i) / (n - 1);
        vertices[2 * n + 1 + i] = Vertex{pi, normal, QVector2D(texX, 0.5f)};
    }

    for (int i = n; i < 2 * n - 1; i++)
    {
        triangles[i].tri = QVector3D(i + 1, i + 2, i + 1 + n);
    }

    triangles[2 * n - 1].tri = QVector3D(2 * n, n + 1, 3 * n);

    for (int i = 2 * n; i < 3 * n - 1; i++)
    {
        triangles[i].tri = QVector3D(i + 1, i + 2 - n, i + 2);
    }

    triangles[3 * n - 1].tri = QVector3D(3 * n, n + 1, 2 * n + 1);

    for (int i = 0; i < triangles.size(); ++i)
    {
        triangles[i].color = QColor::fromHsv(QRandomGenerator::global()->bounded(360), 200, 200);
    }
}

void ShapeViewer::loadTexture(const QString &path)
{
    textureImage = QImage(path).convertToFormat(QImage::Format_RGB32);
    displayMode = DisplayMode::Textured;
    update();
}

void ShapeViewer::setDisplayMode(DisplayMode mode)
{
    displayMode = mode;
    update();
}