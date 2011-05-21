#include "lightgraphicsitem.h"

#include <QtGui>

CLightGraphicsItem::CLightGraphicsItem(float r, QGraphicsItem *parent) :
    QGraphicsItem(parent), radius(r)
{
}

float CLightGraphicsItem::intensityAt(const QPointF &p,
                          const QList<QPolygonF> &obstacles) const
{
    const QPointF c(pos() + QPointF(radius, radius));
    const QLineF line(c, p);
    const float d = line.length();
    if (d > radius)
        return 0.0;

    foreach (QPolygonF obpoly, obstacles)
    {
        if (obpoly.containsPoint(c, Qt::OddEvenFill) ||
            obpoly.containsPoint(p, Qt::OddEvenFill))
            return 0.0;

        for (QPolygonF::iterator it=obpoly.begin(); it!=(obpoly.end()-1); ++it)
        {
            QLineF obl(*it, *(it+1));
            if (line.intersect(obl, 0) == QLineF::BoundedIntersection)
                return 0.0;
        }
    }

    const float maxint = 2.0;
    return ((radius - d) / radius) * maxint;
}

void CLightGraphicsItem::paint(QPainter *painter,
                               const QStyleOptionGraphicsItem *,
                               QWidget *)
{
    QRadialGradient g(radius, radius, radius);
    g.setColorAt(0.0, QColor(255, 255, 0, 200));
    g.setColorAt(1.0, QColor(255, 180, 0, 50));
    painter->setBrush(g);
    painter->drawEllipse(boundingRect());
}
