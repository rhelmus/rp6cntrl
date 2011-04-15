#ifndef PROJECTWIZARD_H
#define PROJECTWIZARD_H

#include <QWizard>

class QTreeWidget;

class CProjectWizard : public QWizard
{
public:
    explicit CProjectWizard(QWidget *parent = 0);

    void accept(void);
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
    QTreeWidget *driverTreeWidget;

    bool checkPermissions(const QString &file) const;

public:
    explicit CNewProjectSettingsPage(QWidget *parent = 0);

    void initializePage();
    bool isComplete(void) const;
    bool validatePage();
};

#endif // PROJECTWIZARD_H
