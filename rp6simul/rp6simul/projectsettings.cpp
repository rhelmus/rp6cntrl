#include "projectsettings.h"
#include "rp6simul.h"

#include <QLibrary>
#include <QMessageBox>

CProjectSettings::CProjectSettings(const QString &file, QObject *parent) :
    QSettings(file, QSettings::IniFormat, parent)
{
}


bool verifyPluginFile(const QString &file)
{
    QString msg;
    if (file.isEmpty())
        msg = "No plugin file specified.";
    else if (!QFileInfo(file).isReadable())
        msg = "You do not have sufficient permissions for "
              "the RP6 plugin specified.";
    else if (!QLibrary::isLibrary(file))
        msg = "Incorrect file type selected.";

    if (!msg.isEmpty())
    {
        QMessageBox::warning(CRP6Simulator::getInstance(), "RP6 plugin error",
                             msg);
        return false;
    }

    return true;
}
