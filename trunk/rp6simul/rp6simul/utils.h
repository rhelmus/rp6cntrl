#ifndef UTILS_H
#define UTILS_H

#include "clock.h"

#include <time.h>

#include <QLibrary>

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

QString getResourcePath(const QString &file=QString());
QString getLuaSrcPath(const QString &file=QString());
QString getMapTemplatesPath(const QString &file=QString());

bool verifySettingsFile(QSettings &file, bool silent=false);
void drawLED(QPainter &painter, CLED *led, const QTransform &tr,
             qreal scale);
qreal toClockwiseAngle(qreal a);
qreal toCounterClockwiseAngle(qreal a);

template <typename TF> TF getLibFunc(QLibrary &lib, const char *f)
{
    TF ret = (TF)lib.resolve(f);
    return ret;
}

#endif // UTILS_H
