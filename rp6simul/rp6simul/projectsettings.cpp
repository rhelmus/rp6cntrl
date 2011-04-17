#include "projectsettings.h"

CProjectSettings::CProjectSettings(const QString &file, QObject *parent) :
    QSettings(file, QSettings::NativeFormat, parent)
{
}
