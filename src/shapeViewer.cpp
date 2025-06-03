#include "include/shapeViewer.h"
#include <QPainter>
#include <QTimer>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector4D>
#include <QVector>
#include <QPainterPath>
#include <QPair>
#include <QMouseEvent>
#include <cmath>
#include <iostream>
#include <QtMath>
#include <QTime>
#include <QRandomGenerator>
#include <QPolygonF>

ShapeViewer::ShapeViewer(QWidget *parent)
    : QWidget(parent)
{
    baseRadius = 50;
    cylinderHeight = 100;
    subDivisons = 20;

    cameraPos = QVector4D(300.0f, 200.0f, 50.0f, 1.0f);
    lookAt = QVector4D(0.0f, cylinderHeight / 2, 0.0f, 1.0f);
    upVec = QVector4D(0.0f, 1.0f, 0.0f, 0.0f);
    buildGeometry();
}

void ShapeViewer::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen); // No outline for filled triangles

    QMatrix4x4 view = getViewMatrix(cameraPos.toVector3D(), lookAt.toVector3D(), upVec.toVector3D());
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

    for (int i = 0; i < triangles.size(); i++)
    {
        const auto &tri = triangles[i];
        int i1 = int(tri.x()), i2 = int(tri.y()), i3 = int(tri.z());

        QVector3D p1 = vertices[i1].position.toVector3D();
        QVector3D p2 = vertices[i2].position.toVector3D();
        QVector3D p3 = vertices[i3].position.toVector3D();
        QVector3D normal = QVector3D::normal(p1, p2, p3);

        if (!isFacingFront(normal, cameraPos.toVector3D(), lookAt.toVector3D()))
            continue;

        QPolygonF poly;
        poly << projected[i1] << projected[i2] << projected[i3];

        if (useTexture && textureImage.isNull() == false)
        {
            remapTriangle(painter,
                          projected[i1], projected[i2], projected[i3],
                          vertices[i1].textureCoord,
                          vertices[i2].textureCoord,
                          vertices[i3].textureCoord,
                          textureImage,
                          Qt::black);
        }
        else
        {
            painter.setBrush(triangleColors[i]);
        }

        painter.drawPolygon(poly);
    }
}

QMatrix4x4 ShapeViewer::getViewMatrix(const QVector3D &cameraPos, const QVector3D &lookAt, const QVector3D &upVec)
{
    QVector3D cZ = (cameraPos - lookAt).normalized();
    QVector3D cX = QVector3D::crossProduct(upVec, cZ).normalized();
    QVector3D cY = QVector3D::crossProduct(cZ, cX).normalized();

    QMatrix4x4 viewMatrix;
    viewMatrix.setRow(0, QVector4D(cX.x(), cX.y(), cX.z(), -QVector3D::dotProduct(cX, cameraPos)));
    viewMatrix.setRow(1, QVector4D(cY.x(), cY.y(), cY.z(), -QVector3D::dotProduct(cY, cameraPos)));
    viewMatrix.setRow(2, QVector4D(cZ.x(), cZ.y(), cZ.z(), -QVector3D::dotProduct(cZ, cameraPos)));
    viewMatrix.setRow(3, QVector4D(0.0f, 0.0f, 0.0f, 1.0f));

    return viewMatrix;
}

QMatrix4x4 ShapeViewer::getProjectionMatrix(float Sx, float Sy, float theta)
{
    float cot = 1.0f / tan(theta * M_PI / 360.0f); // cot(theta/2) = 1/tan(theta/2)
    float Sx_half = Sx / 2.0f;
    float Sy_half = Sy / 2.0f;
    QMatrix4x4 projectionMatrix;
    projectionMatrix.setRow(0, QVector4D(-Sx_half * cot, 0.0f, Sx_half, 0.0f));
    projectionMatrix.setRow(1, QVector4D(0.0f, Sy_half * cot, Sy_half, 0.0f));
    projectionMatrix.setRow(2, QVector4D(0.0f, 0.0f, 0.0f, 1.0f));
    projectionMatrix.setRow(3, QVector4D(0.0f, 0.0f, 1.0f, 0.0f));

    return projectionMatrix;
}

QMatrix4x4 ShapeViewer::getRotationMatrix(const QVector3D &axis, float angleDegrees)
{
    QMatrix4x4 mat;
    QVector3D a = axis.normalized();
    float angle = qDegreesToRadians(angleDegrees);
    float c = cos(angle);
    float s = sin(angle);
    float one_c = 1.0f - c;

    float x = a.x(), y = a.y(), z = a.z();

    mat.setRow(0, QVector4D(c + x * x * one_c, x * y * one_c - z * s, x * z * one_c + y * s, 0));
    mat.setRow(1, QVector4D(y * x * one_c + z * s, c + y * y * one_c, y * z * one_c - x * s, 0));
    mat.setRow(2, QVector4D(z * x * one_c - y * s, z * y * one_c + x * s, c + z * z * one_c, 0));
    mat.setRow(3, QVector4D(0, 0, 0, 1));

    return mat;
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

    QVector3D camVec = cameraPos.toVector3D() - lookAt.toVector3D();

    // Rotation around world Y (horizontal drag)
    QMatrix4x4 rotY = getRotationMatrix(QVector3D(0, 1, 0), -delta.x() * rotationSpeed);

    // Rotation around camera's right vector (vertical drag)
    QVector3D right = QVector3D::crossProduct(upVec.toVector3D(), camVec).normalized();
    QMatrix4x4 rotRight = getRotationMatrix(right.normalized(), -delta.y() * rotationSpeed);

    QVector3D newCamVec = rotRight * rotY * camVec;
    cameraPos = QVector4D(lookAt.toVector3D() + newCamVec, 1.0f);

    update();
}

void ShapeViewer::wheelEvent(QWheelEvent *event)
{
    float delta = event->angleDelta().y() / 120.0f;

    float zoomFactor = std::pow(zoomSpeed, -delta);

    QVector3D newPos = lookAt.toVector3D() + (cameraPos.toVector3D() - lookAt.toVector3D()) * zoomFactor;

    // Clamp to avoid crossing the origin or going too far
    float minDist = 20.0f;
    float maxDist = 2000.0f;
    float dist = (newPos - lookAt.toVector3D()).length();
    if (dist > minDist && dist < maxDist)
        cameraPos = QVector4D(newPos, 1.0f);

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
    triangles = QVector<QVector3D>(4 * n);
    triangleColors = QVector<QColor>(4 * n);

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
        triangles[i] = QVector3D(0, i + 2, i + 1);
    }

    triangles[n - 1] = QVector3D(0, 1, n);

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
        triangles[i] = QVector3D(4 * n + 1, i + 1, i + 2);
    }

    triangles[4 * n - 1] = QVector3D(4 * n + 1, 4 * n, 3 * n + 1);

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
        triangles[i] = QVector3D(i + 1, i + 2, i + 1 + n);
    }

    triangles[2 * n - 1] = QVector3D(2 * n, n + 1, 3 * n);

    for (int i = 2 * n; i < 3 * n - 1; i++)
    {
        triangles[i] = QVector3D(i + 1, i + 2 - n, i + 2);
    }

    triangles[3 * n - 1] = QVector3D(3 * n, n + 1, 2 * n + 1);

    for (int i = 0; i < triangles.size(); ++i)
    {
        triangleColors[i] = QColor::fromHsv(QRandomGenerator::global()->bounded(360), 200, 200);
    }
}

void ShapeViewer::loadTexture(const QString &path)
{
    textureImage = QImage(path).convertToFormat(QImage::Format_RGB32);
    useTexture = !textureImage.isNull();
    update();
}

void ShapeViewer::toggleUseTexture()
{
    useTexture = !useTexture;
    update();
}

void ShapeViewer::remapTriangle(QPainter &painter, const QPointF &p1, const QPointF &p2, const QPointF &p3,
                                const QVector2D &t1, const QVector2D &t2, const QVector2D &t3,
                                const QImage &texture, const QColor &borderColor)
{
    QRectF bbox = QPolygonF({p1, p2, p3}).boundingRect();
    int minX = std::max(0, int(std::floor(bbox.left())));
    int maxX = std::min(width() - 1, int(std::ceil(bbox.right())));
    int minY = std::max(0, int(std::floor(bbox.top())));
    int maxY = std::min(height() - 1, int(std::ceil(bbox.bottom())));

    for (int y = minY; y <= maxY; ++y)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            QPointF P(x + 0.5, y + 0.5);

            float denom = (p2.y() - p3.y()) * (p1.x() - p3.x()) +
                          (p3.x() - p2.x()) * (p1.y() - p3.y());

            if (denom == 0.0f)
                continue;

            float w1 = ((p2.y() - p3.y()) * (P.x() - p3.x()) +
                        (p3.x() - p2.x()) * (P.y() - p3.y())) /
                       denom;
            float w2 = ((p3.y() - p1.y()) * (P.x() - p3.x()) +
                        (p1.x() - p3.x()) * (P.y() - p3.y())) /
                       denom;
            float w3 = 1.0f - w1 - w2;

            if (w1 < 0 || w2 < 0 || w3 < 0)
                continue;

            QVector2D texCoord = w1 * t1 + w2 * t2 + w3 * t3;

            int tx = int(texCoord.x() * texture.width());
            int ty = int(texCoord.y() * texture.height());

            QColor color;
            if (tx >= 0 && tx < texture.width() && ty >= 0 && ty < texture.height())
            {
                color = texture.pixelColor(tx, ty);
            }
            else
            {
                color = borderColor;
            }

            painter.setPen(color);
            painter.drawPoint(x, y);
        }
    }
}
