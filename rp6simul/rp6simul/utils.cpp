#include "utils.h"

#include "rp6simul.h"

#include <QMessageBox>
#include <QSettings>
#include <QString>

bool checkSettingsFile(QSettings &file)
{
    file.sync();

    if (file.status() == QSettings::AccessError)
        QMessageBox::critical(CRP6Simulator::getInstance(), "File access error", "Unable to access project file!");
    else if (file.status() == QSettings::FormatError)
        QMessageBox::critical(CRP6Simulator::getInstance(), "File format error", "Invalid file format!");
    else
        return true;

    return false;
}
