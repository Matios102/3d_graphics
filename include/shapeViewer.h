#pragma once

#include <QWidget>
#include <QPointF>
#include <QVector3D>
#include <QVector4D>
#include <QVector2D>
#include <QTimer>
#include "camera.h"

enum class DisplayMode
{
    Wireframe,
    Colored,
    Textured
};

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
    void setDisplayMode(DisplayMode mode);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    bool isFacingFront(const QVector3D &normal, const QVector3D &cameraPos, const QVector3D &lookAt);

private:
    void buildGeometry();

    // Cylinder parameters
    int baseRadius = 10;
    int cylinderHeight = 20;
    int subDivisons = 10;

    QVector<Vertex> vertices;
    QVector<Triangle> triangles;

    Camera camera;

    // Mouse interaction
    QPoint lastMousePos;


    QImage textureImage;
    DisplayMode displayMode = DisplayMode::Wireframe;
};
