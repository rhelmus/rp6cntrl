#ifndef PROJECTWIZARD_H
#define PROJECTWIZARD_H

#include <QMap>
#include <QWizard>

class QTreeWidget;

class CNewProjectSettingsPage;

class CProjectWizard : public QWizard
{
    CNewProjectSettingsPage *projectSettingsPage;

public:
    explicit CProjectWizard(QWidget *parent = 0);

    void accept(void);

    QString getProjectFile(void) const;
};

class CNewProjectDestPage: public QWizardPage
{
public:
    explicit CNewProjectDestPage(QWidget *parent = 0);

    void initializePage();
    bool validatePage();
};

class CNewProjectSettingsPage: public QWizardPage
{
    Q_OBJECT

    QTreeWidget *driverTreeWidget;
    QMap<QString, QString> driverList;
    QStringList defaultDrivers;
    bool initDriverList;
    QPushButton *addDriverButton, *delDriverButton;
    QAction *addCustomDriverAction;

    void getDriverList(void);
    QAction *getAddAction(const QString &driver);
    bool checkPermissions(const QString &file) const;

private slots:
    void addDriver(QAction *action);
    void delDriver(void);
    void resetDriverTree(void);

public:
    explicit CNewProjectSettingsPage(QWidget *parent = 0);

    void initializePage();
    bool isComplete(void) const;
    bool validatePage();

    QStringList getSelectedDrivers(void) const;
};

#endif // PROJECTWIZARD_H
