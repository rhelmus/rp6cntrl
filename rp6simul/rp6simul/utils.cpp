#include "utils.h"

#include <QString>

const char *getCString(const QString &s)
{
    static QByteArray by;
    by = s.toLatin1();
    return by.data();
}

