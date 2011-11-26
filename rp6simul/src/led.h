#ifndef LED_H
#define LED_H

#include <QColor>
#include <QPointF>

class CLED
{
    const QPointF position;
    const QColor color;
    const float radius;
    bool enabled;

public:
    CLED(const QPointF &p, const QColor &c, float r) : position(p), color(c),
        radius(r), enabled(false) { }

    QPointF getPosition(void) const { return position; }
    QColor getColor(void) const { return color; }
    float getRadius(void) const { return radius; }
    bool isEnabled(void) const { return enabled; }
    void setEnabled(bool e) { enabled = e; }
};

#endif // LED_H
