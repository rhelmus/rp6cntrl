#ifndef BUMPER_H
#define BUMPER_H

#include <QColor>
#include <QPolygonF>

class CBumper
{
    const QPolygonF points;
    const QColor color;
    bool hit;

public:
    CBumper(const QPolygonF &p, const QColor &c) : points(p), color(c),
        hit(false) { }

    QPolygonF getPoints(void) const { return points; }
    QColor getColor(void) const { return color; }
    bool isHit(void) const { return hit; }
    void setHit(bool e) { hit = e; }
};

#endif // BUMPER_H
