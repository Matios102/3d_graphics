#pragma once
#include <QVector3D>

class Camera
{
    QVector3D position;
    QVector3D lookAt;
    QVector3D upVector;

public:
    Camera(const QVector3D &pos, const QVector3D &look, const QVector3D &up)
        : position(pos), lookAt(look), upVector(up) {}
    
    Camera() : position(0.0f, 0.0f, 100.0f), lookAt(0.0f, 0.0f, 0.0f), upVector(0.0f, 1.0f, 0.0f) {}

    QVector3D getPosition() const { return position; }
    QVector3D getLookAt() const { return lookAt; }
    QVector3D getUpVector() const { return upVector; }

    void setLookAt(const QVector3D &look) { lookAt = look; }

    void updatePosition(QPoint delta);
    void zoom(float delta);

private:
    float zoomSpeed = 1.1f;
    float rotationSpeed = 0.5f;
};