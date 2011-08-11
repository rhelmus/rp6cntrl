#ifndef UTILS_H
#define UTILS_H

#include "clock.h"

#include <time.h>

class QPainter;
class QPoint;
class QSettings;
class QString;

inline unsigned long getUSDiff(const timespec &start, const timespec &end)
{
    return ((end.tv_sec-start.tv_sec) * 1000000) +
            ((end.tv_nsec-start.tv_nsec) / 1000);
}

bool verifySettingsFile(QSettings &file);
void drawLED(QPainter &painter, const char *propname,
             const QPoint &base, qreal scale);

#endif // UTILS_H
