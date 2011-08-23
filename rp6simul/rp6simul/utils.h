#ifndef UTILS_H
#define UTILS_H

#include "clock.h"

#include <time.h>

class QPainter;
class QSettings;
class QString;
class QTransform;

class CLED;

inline unsigned long getUSDiff(const timespec &start, const timespec &end)
{
    return ((end.tv_sec-start.tv_sec) * 1000000) +
            ((end.tv_nsec-start.tv_nsec) / 1000);
}

bool verifySettingsFile(QSettings &file);
void drawLED(QPainter &painter, const char *propname,
             const QTransform &tr, qreal scale);
void drawLED(QPainter &painter, CLED *led, const QTransform &tr,
             qreal scale);

#endif // UTILS_H
