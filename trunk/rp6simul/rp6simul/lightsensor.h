#ifndef LIGHTSENSOR_H
#define LIGHTSENSOR_H

#include <QMutex>
#include <QMutexLocker>

class CLightSensor
{
    const QPointF position;
    int light;
    mutable QMutex lightMutex;

public:
    CLightSensor(const QPointF &p) : position(p), light(0),
        lightMutex(QMutex::Recursive) { }

    QPointF getPosition(void) const { return position; }
    float getLight(void) const
    {
        QMutexLocker l(&lightMutex);
        const int ret = light;
        return ret;
    }
    void setLight(int l)
    { QMutexLocker lk(&lightMutex); light = l; }
};

#endif // LIGHTSENSOR_H
