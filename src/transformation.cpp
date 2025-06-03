#include "transformation.h"
#include <QtMath>

QMatrix4x4 getViewMatrix(const QVector3D &cameraPos, const QVector3D &lookAt, const QVector3D &upVec)
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

QMatrix4x4 getProjectionMatrix(float Sx, float Sy, float theta)
{
    float cot = 1.0f / tan(theta * M_PI / 360.0f);
    float Sx_half = Sx / 2.0f;
    float Sy_half = Sy / 2.0f;
    QMatrix4x4 projectionMatrix;
    projectionMatrix.setRow(0, QVector4D(-Sx_half * cot, 0.0f, Sx_half, 0.0f));
    projectionMatrix.setRow(1, QVector4D(0.0f, Sy_half * cot, Sy_half, 0.0f));
    projectionMatrix.setRow(2, QVector4D(0.0f, 0.0f, 0.0f, 1.0f));
    projectionMatrix.setRow(3, QVector4D(0.0f, 0.0f, 1.0f, 0.0f));

    return projectionMatrix;
}

QMatrix4x4 getRotationMatrix(const QVector3D &axis, float angleDegrees)
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
