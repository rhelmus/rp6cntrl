#ifndef PROJECTSETTINGS_H
#define PROJECTSETTINGS_H

#include "rp6simul.h"

#include <QDialog>
#include <QDir>
#include <QSettings>

class QAction;
class QButtonGroup;
class QComboBox;
class QPushButton;
class QTreeWidget;

class CDriverSelectionWidget;
class CPathInput;

class CProjectSettings : public QSettings
{
public:
    CProjectSettings(const QString &file, QObject *parent = 0)
        : QSettings(file, QSettings::IniFormat, parent) { }
};


class CProjectSettingsDialog : public QDialog
{
    Q_OBJECT

    QWidget *robotTab, *m32Tab;
    QButtonGroup *simulatorButtonGroup;
    CPathInput *robotPluginInput, *m32PluginInput;
    CDriverSelectionWidget *robotDriverWidget, *m32DriverWidget;
    QComboBox *m32SlotComboBox;

    QWidget *createMainTab(void);
    QWidget *createRobotTab(void);
    QWidget *createM32Tab(void);

private slots:
    void updateSimulatorSelection(int simulator);

public:
    CProjectSettingsDialog(QWidget *parent=0, Qt::WindowFlags f=0);

    void loadSettings(CProjectSettings &settings);
    void saveSettings(CProjectSettings &settings);

public slots:
    void accept();
};

class CDriverSelectionWidget : public QWidget
{
    Q_OBJECT

    TDriverInfoList driverInfoList;
    QTreeWidget *driverTreeWidget;
    QPushButton *addDriverButton, *delDriverButton;
    QAction *addCustomDriverAction;

    void initDrivers(void);
    QAction *getAddAction(const QString &driver);
    void addCustomDriver(const QString &file);

private slots:
    void addDriver(QAction *action);
    void delDriver(void);
    void resetDriverTree(void);

public:
    CDriverSelectionWidget(const TDriverInfoList &drlist, QWidget *parent=0,
                           Qt::WindowFlags f=0);

    void selectDrivers(QStringList drivers);
    QStringList getSelectedDrivers(void) const;
};


// Utils
inline QString projectFilePath(const QString &dir, const QString &name)
{
    return QDir(dir).absoluteFilePath(name + ".rp6");
}

inline const char *getPluginFilter(void)
{
#ifdef Q_OS_WIN
    return "plugin files (*.dll)";
#else
    return "plugin files (*.so)";
#endif
}

bool verifyPluginFile(const QString &file);

#endif // PROJECTSETTINGS_H
