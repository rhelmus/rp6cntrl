#ifndef UTILS_H
#define UTILS_H

#include "clock.h"

#include <time.h>

class QSettings;
class QString;

const char *getCString(const QString &s);

inline unsigned long getUSDiff(const timespec &start, const timespec &end)
{
    return ((end.tv_sec-start.tv_sec) * 1000000) +
            ((end.tv_nsec-start.tv_nsec) / 1000);
}

bool checkSettingsFile(QSettings &file);

#endif // UTILS_H
