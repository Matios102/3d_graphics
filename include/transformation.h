#pragma once

#include <QMatrix4x4>
#include <QVector3D>

QMatrix4x4 getViewMatrix(const QVector3D &cameraPos, const QVector3D &lookAt, const QVector3D &upVec);
QMatrix4x4 getProjectionMatrix(float Sx, float Sy, float theta);
QMatrix4x4 getRotationMatrix(const QVector3D &axis, float angleDegrees);