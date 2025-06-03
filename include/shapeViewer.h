#pragma once

#include <QWidget>
#include <QPointF>
#include <QVector3D>
#include <QVector4D>
#include <QVector2D>
#include <QTimer>

struct Vertex
{
    QVector4D position;
    QVector4D normal;
    QVector2D textureCoord;
};

struct Triangle
{
    QVector3D tri;
    float avgDistance;
    QColor color;
};

class ShapeViewer : public QWidget
{
    Q_OBJECT

public:
    ShapeViewer(QWidget *parent = nullptr);

public slots:
    void setBaseRadius(int r);
    void setHeight(int h);
    void setSubdivisions(int s);
    void loadTexture(const QString &path);
    void toggleUseTexture();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

    QMatrix4x4 getRotationMatrix(const QVector3D &axis, float angleDegrees);
    QMatrix4x4 getViewMatrix(const QVector3D &cameraPos, const QVector3D &lookAt, const QVector3D &upVec);
    QMatrix4x4 getProjectionMatrix(float Sx, float Sy, float theta);
    bool isFacingFront(const QVector3D &normal, const QVector3D &cameraPos, const QVector3D &lookAt);

    void remapTriangle(QPainter &painter, const QPointF &p1, const QPointF &p2, const QPointF &p3,
                                const QVector2D &t1, const QVector2D &t2, const QVector2D &t3,
                                const QImage &texture, const QColor &borderColor);

private:
    void buildGeometry();

    // Cylinder parameters
    int baseRadius = 10;
    int cylinderHeight = 20;
    int subDivisons = 10;

    QVector<Vertex> vertices;
    QVector<Triangle> triangles;

    // Camera parameters
    QVector4D cameraPos, lookAt, upVec;

    // Mouse interaction
    QPoint lastMousePos;
    float zoomSpeed = 1.1f;
    float rotationSpeed = 0.5f;

    QImage textureImage;
    bool useTexture = false;
};
