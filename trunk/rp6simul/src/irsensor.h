#ifndef IRSENSOR_H
#define IRSENSOR_H

#include <QMutex>
#include <QMutexLocker>

class CIRSensor
{
    const QPointF position;
    const QColor color;
    const float radius;
    const float traceAngle;
    const float traceDistance;
    float hitDistance;
    mutable QMutex hitDistanceMutex;

public:
    CIRSensor(const QPointF &p, const QColor &c, float r, float a, float d)
        : position(p), color(c), radius(r), traceAngle(a), traceDistance(d),
          hitDistance(d), hitDistanceMutex(QMutex::Recursive) { }

    QPointF getPosition(void) const { return position; }
    QColor getColor(void) const { return color; }
    float getRadius(void) const { return radius; }
    float getTraceAngle(void) const { return traceAngle; }
    float getTraceDistance(void) const { return traceDistance; }
    float getHitDistance(void) const
    {
        QMutexLocker l(&hitDistanceMutex);
        const float ret = hitDistance;
        return ret;
    }
    void setHitDistance(float h)
    { QMutexLocker l(&hitDistanceMutex); hitDistance = h; }
};

#endif // IRSENSOR_H
