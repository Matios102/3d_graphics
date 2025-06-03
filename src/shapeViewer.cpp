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

    // Draw triangles with back-face culling and random colors
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

        QColor color = triangleColors[i];
        painter.setBrush(color);
        painter.drawPolygon(poly);
    }

    // Optional: draw vertices as small circles
    // painter.setBrush(Qt::black);
    // for (const auto &pt : projected)
    //     painter.drawEllipse(pt, 2, 2);
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

void ShapeViewer::setBaseRadius(int r) {
    baseRadius = r;
    buildGeometry();
    update();
}

void ShapeViewer::setHeight(int h) {
    cylinderHeight = h;
    buildGeometry();
    update();
}

void ShapeViewer::setSubdivisions(int s) {
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
    triangleColors = QVector<QColor>(4*n);

    QVector4D tex(0, 0, 0, 1);

    // === Top ===
    vertices[0] = Vertex{QVector4D(0, h, 0, 1), QVector4D(0, 1, 0, 0), tex};

    for (int i = 0; i < n; ++i)
    {
        float angle = 2 * M_PI * i / n;
        float x = r * cos(angle);
        float z = r * sin(angle);
        vertices[i + 1] = Vertex{QVector4D(x, h, z, 1), QVector4D(0, 1, 0, 0), tex};
    }

    for (int i = 0; i < n - 1; i++)
    {
        triangles[i] = QVector3D(0, i + 2, i + 1);
    }

    triangles[n - 1] = QVector3D(0, 1, n);

    // === Bottom ===
    vertices[4 * n + 1] = Vertex{QVector4D(0, 0, 0, 1), QVector4D(0, -1, 0, 0), tex};

    for (int i = 0; i < n; ++i)
    {
        float angle = 2 * M_PI * i / n;
        float x = r * cos(angle);
        float z = r * sin(angle);
        vertices[3 * n + i + 1] = Vertex{QVector4D(x, 0, z, 1), QVector4D(0, -1, 0, 0), tex};
    }

    for (int i = 3 * n; i < 4 * n - 1; i++)
    {
        triangles[i] = QVector3D(4 * n + 1, i + 1, i + 2);
    }

    triangles[4 * n - 1] = QVector3D(4 * n + 1, 4 * n, 3 * n + 1);

    // === Side ===
    for (int i = n + 1; i < 2 * n + 1; ++i)
    {
        QVector4D pi = vertices[i - n].position;
        QVector4D normal = QVector4D(pi.x() / r, 0, pi.z() / r, 0);
        vertices[i] = Vertex{pi, normal, tex};
    }

    for (int i = 2 * n + 1; i < 3 * n + 1; ++i)
    {
        QVector4D pi = vertices[i + n].position;
        QVector4D normal = QVector4D(pi.x() / r, 0, pi.z() / r, 0);
        vertices[i] = Vertex{pi, normal, tex};
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
