#ifndef PROJECTSETTINGS_H
#define PROJECTSETTINGS_H

#include <QSettings>
#include <QDir>

class CProjectSettings : public QSettings
{
public:
    CProjectSettings(const QString &file, QObject *parent = 0);
};

inline QString projectFilePath(const QString &dir, const QString &name)
{
    return QDir(dir).absoluteFilePath(name + ".rp6");
}

#endif // PROJECTSETTINGS_H
