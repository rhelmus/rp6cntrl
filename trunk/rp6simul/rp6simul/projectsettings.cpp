#include "projectsettings.h"
#include "rp6simul.h"

#include <QtGui>

CProjectSettings::CProjectSettings(const QString &file, QObject *parent) :
    QSettings(file, QSettings::NativeFormat, parent)
{
}


bool checkProjectSettings(CProjectSettings &prs)
{
    prs.sync();

    if (prs.status() == QSettings::AccessError)
        QMessageBox::critical(CRP6Simulator::getInstance(), "File access error", "Unable to access project file!");
    else if (prs.status() == QSettings::FormatError)
        QMessageBox::critical(CRP6Simulator::getInstance(), "File format error", "Invalid file format!");
    else
        return true;

    return false;
}
