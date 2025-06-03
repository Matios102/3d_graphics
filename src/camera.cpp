#include "camera.h"
#include "transformation.h"


void Camera::updatePosition(QPoint delta)
{
    QVector3D camVec = position - lookAt;

    // Rotation around world Y (horizontal drag)
    QMatrix4x4 rotY = getRotationMatrix(QVector3D(0, 1, 0), -delta.x() * rotationSpeed);

    // Rotation around camera's right vector (vertical drag)
    QVector3D right = QVector3D::crossProduct(upVector, camVec).normalized();
    QMatrix4x4 rotRight = getRotationMatrix(right.normalized(), -delta.y() * rotationSpeed);

    QVector3D newCamVec = rotRight * rotY * camVec;
    position = QVector3D(lookAt + newCamVec);
}

void Camera::zoom(float delta)
{
    float zoomFactor = std::pow(zoomSpeed, -delta);

    QVector3D newPos = lookAt + (position - lookAt) * zoomFactor;

    // Clamp to avoid crossing the origin or going too far
    float minDist = 20.0f;
    float maxDist = 2000.0f;
    float dist = (newPos - lookAt).length();
    if (dist > minDist && dist < maxDist)
        position = newPos;
}