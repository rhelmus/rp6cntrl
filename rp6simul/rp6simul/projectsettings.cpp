#include "projectsettings.h"
#include "rp6simul.h"

CProjectSettings::CProjectSettings(const QString &file, QObject *parent) :
    QSettings(file, QSettings::IniFormat, parent)
{
}
