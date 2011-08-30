#ifndef LIGHTSENSOR_H
#define LIGHTSENSOR_H

#ifdef USEATOMIC
#include <QAtomicInt>
#else
#include <QMutex>
#include <QMutexLocker>
#endif

class CLightSensor
{
    const QPointF position;
#ifdef USEATOMIC
    QAtomicInt light;
#else
    int light;
    mutable QMutex lightMutex;
#endif

public:
    CLightSensor(const QPointF &p) : position(p), light(0)
  #ifndef USEATOMIC
      , lightMutex(QMutex::Recursive)
  #endif
    { }

    QPointF getPosition(void) const { return position; }
#ifdef USEATOMIC
    int getLight(void) const { return light; }
    void setLight(int l) { light = l; }
#else
    int getLight(void) const
    {
        QMutexLocker l(&lightMutex);
        const int ret = light;
        return ret;
    }
    void setLight(int l)
    { QMutexLocker lk(&lightMutex); light = l; }
#endif
};

#endif // LIGHTSENSOR_H
