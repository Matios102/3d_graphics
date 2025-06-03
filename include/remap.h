#pragma once

#include <QImage>
#include <QPointF>
#include <QVector2D>
#include <QColor>

void remapTriangleToImage(QImage &img,
                                       const QPointF &p1, const QPointF &p2, const QPointF &p3,
                                       const QVector2D &t1, const QVector2D &t2, const QVector2D &t3,
                                       const QImage &texture,
                                       const QColor &defaultColor)
{
    QRectF bbox = QPolygonF({p1, p2, p3}).boundingRect();
    int minX = std::max(0, int(std::floor(bbox.left())));
    int maxX = std::min(img.width() - 1, int(std::ceil(bbox.right())));
    int minY = std::max(0, int(std::floor(bbox.top())));
    int maxY = std::min(img.height() - 1, int(std::ceil(bbox.bottom())));

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

            QColor color = defaultColor;
            if (!texture.isNull())
            {
                QVector2D texCoord = w1 * t1 + w2 * t2 + w3 * t3;
                int tx = int(texCoord.x() * texture.width());
                int ty = int(texCoord.y() * texture.height());
                if (tx >= 0 && tx < texture.width() && ty >= 0 && ty < texture.height())
                    color = texture.pixelColor(tx, ty);
            }

            img.setPixel(x, y, color.rgb());
        }
    }
}
