#ifndef PROJECTWIZARD_H
#define PROJECTWIZARD_H

#include "rp6simul.h"

#include <QHash>
#include <QWizard>

class QButtonGroup;
class QComboBox;
class QTreeWidget;

class CNewProjectSimulatorPage;

class CProjectWizard : public QWizard
{
    CNewProjectSimulatorPage *projectSimulatorPage;

public:
    enum { PAGE_PROJDEST, PAGE_SIMULATOR, PAGE_ROBOT, PAGE_M32 };

    explicit CProjectWizard(QWidget *parent = 0);

    void accept(void);

    QString getProjectFile(void) const;
};

class CNewProjectDestPage : public QWizardPage
{
public:
    explicit CNewProjectDestPage(QWidget *parent = 0);

    void initializePage();
    bool validatePage();
};

class CNewProjectSimulatorPage : public QWizardPage
{
    QButtonGroup *simulatorButtonGroup;

public:
    explicit CNewProjectSimulatorPage(QWidget *parent = 0);

    int nextId() const;

    ESimulator getSimulator(void) const;
};

class CNewProjectRobotPage : public QWizardPage
{
public:
    explicit CNewProjectRobotPage(QWidget *parent = 0);

    void cleanupPage() { }
    bool isComplete(void) const;
    int nextId() const;
    bool validatePage();
};

class CNewProjectM32Page : public QWizardPage
{
    QWidget *formSlotLabel;
    QComboBox *slotComboBox;

public:
    explicit CNewProjectM32Page(QWidget *parent = 0);

    void cleanupPage() { }
    void initializePage();
    bool isComplete(void) const;
    bool validatePage();
};

class CNewProjectSettingsPage: public QWizardPage
{
    Q_OBJECT

    QTreeWidget *driverTreeWidget;
    QHash<QString, QString> driverDescList;
    QStringList defaultDriverList;
    bool initDriverList;
    QPushButton *addDriverButton, *delDriverButton;
    QAction *addCustomDriverAction;

    QAction *getAddAction(const QString &driver);

private slots:
    void addDriver(QAction *action);
    void delDriver(void);
    void resetDriverTree(void);

public:
    explicit CNewProjectSettingsPage(QWidget *parent = 0);

    void initializePage();
    bool isComplete(void) const;
    bool validatePage();

    void setDriverLists(const QHash<QString, QString> &desc,
                        const QStringList &def)
    { driverDescList = desc; defaultDriverList = def; }
    QStringList getSelectedDrivers(void) const;
};

#endif // PROJECTWIZARD_H
