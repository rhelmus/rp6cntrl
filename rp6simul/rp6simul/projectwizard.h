#ifndef PROJECTWIZARD_H
#define PROJECTWIZARD_H

#include <QHash>
#include <QWizard>

class QTreeWidget;

class CNewProjectSettingsPage;

class CProjectWizard : public QWizard
{
    CNewProjectSettingsPage *projectSettingsPage;

public:
    explicit CProjectWizard(QWidget *parent = 0);

    void accept(void);

    void setDriverLists(const QHash<QString, QString> &desc,
                        const QStringList &def);
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
