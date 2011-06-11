#include "projectwizard.h"
#include "lua.h"
#include "pathinput.h"
#include "projectsettings.h"
#include "utils.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QtGui>

CProjectWizard::CProjectWizard(QWidget *parent) : QWizard(parent)
{
    setDefaultProperty("CPathInput", "path",
                       SIGNAL(pathTextChanged(const QString &)));
    setModal(true);
    addPage(new CNewProjectDestPage);
    addPage(projectSettingsPage = new CNewProjectSettingsPage);
}

void CProjectWizard::accept()
{
    const QString file = projectFilePath(field("projectDir").toString(),
                                         field("projectName").toString());
    if (QFileInfo(file).exists())
    {
        if (!QFile(file).remove())
        {
            QMessageBox::critical(this, "Remove file error",
                                  "Could not remove existing file!");
            reject();
            return;
        }
    }

    CProjectSettings prsettings(file);
    if (verifySettingsFile(prsettings))
    {
        prsettings.setValue("version", 1);
        prsettings.setValue("name", field("projectName"));
        prsettings.setValue("drivers", projectSettingsPage->getSelectedDrivers());
        prsettings.setValue("RP6Plugin", field("RP6Plugin"));
        QDialog::accept();
    }
    else
        reject();
}

QString CProjectWizard::getProjectFile() const
{
    return projectFilePath(field("projectDir").toString(),
                           field("projectName").toString());
}

CNewProjectDestPage::CNewProjectDestPage(QWidget *parent) : QWizardPage(parent)
{
    setTitle("Project destination");

    QFormLayout *form = new QFormLayout(this);
    form->setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QLabel *label =
            new QLabel("Please specify the project name and "
                       "its destination directory. The project name is also used "
                       "as the filename for the project file. The project "
                       "destination directory is freely choosable and it may "
                       "already be existant. It's used to store just the project only.");
    label->setWordWrap(true);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    form->addRow(label);

    QLineEdit *line = new QLineEdit;
    form->addRow("Project name", line);
    registerField("projectName*", line);

    CPathInput *dirinput = new CPathInput("Select project directory",
                                          CPathInput::PATH_EXISTDIR);
    form->addRow("Project directory", dirinput);
    registerField("projectDir*", dirinput);
}

void CNewProjectDestPage::initializePage()
{
    setField("projectDir", QDir::homePath());
}

bool CNewProjectDestPage::validatePage()
{
    QFileInfo fi(field("projectDir").toString());

    if (!fi.isExecutable() || !fi.isReadable() || !fi.isWritable())
    {
        QMessageBox::critical(this, "Project directory error",
                             "You do not have sufficient permissions for "
                             "the directory specified.");
        return false;
    }

    fi = QFileInfo(projectFilePath(field("projectDir").toString(),
                                   field("projectName").toString()));
    if (fi.isFile())
    {
        QMessageBox::StandardButton button =
                QMessageBox::warning(this, "Project file exists",
                             "A project with the same name already exists in "
                             "the directory specified. Overwrite?",
                             QMessageBox::Yes | QMessageBox::No);
        return (button == QMessageBox::Yes);
    }


    return true;
}


CNewProjectSettingsPage::CNewProjectSettingsPage(QWidget *parent)
    : QWizardPage(parent), initDriverList(true)
{
    setTitle("Project settings");

    // UNDONE: Description?
    // UNDONE: Move to seperate reusable widget?

    QGridLayout *grid = new QGridLayout(this);

    QGroupBox *group = new QGroupBox("RP6 plugin");
    grid->addWidget(group, 0, 0);
    QVBoxLayout *vbox = new QVBoxLayout(group);
    // UNDONE: File filter (.so/.dll)
    CPathInput *pathinput = new CPathInput("Open RP6 plugin file",
                                           CPathInput::PATH_EXISTFILE);
    vbox->addWidget(pathinput);
    registerField("RP6Plugin", pathinput);
    connect(pathinput, SIGNAL(pathTextChanged(const QString &)), this,
            SIGNAL(completeChanged()));

    group = new QGroupBox("Drivers");
    grid->addWidget(group, 1, 0, 1, 2);
    QHBoxLayout *hbox = new QHBoxLayout(group);

    hbox->addWidget(driverTreeWidget = new QTreeWidget);
    driverTreeWidget->setHeaderLabels(QStringList() << "Driver" << "Description");
    driverTreeWidget->setRootIsDecorated(false);

    hbox->addLayout(vbox = new QVBoxLayout);

    vbox->addWidget(addDriverButton = new QPushButton("Add driver"));
    QMenu *menu = new QMenu(addDriverButton);
    menu->addSeparator();
    addCustomDriverAction = menu->addAction("Add custom driver...");
    connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(addDriver(QAction*)));
    addDriverButton->setMenu(menu);

    vbox->addWidget(delDriverButton = new QPushButton("Remove driver"));
    connect(delDriverButton, SIGNAL(clicked()), this, SLOT(delDriver()));

    QPushButton *button = new QPushButton("Reset");
    connect(button, SIGNAL(clicked()), this, SLOT(resetDriverTree()));
    vbox->addWidget(button);
}

void CNewProjectSettingsPage::getDriverList()
{
    lua_getglobal(NLua::luaInterface, "getDriverList");
    lua_call(NLua::luaInterface, 0, 2);

    luaL_checktype(NLua::luaInterface, -2, LUA_TTABLE);
    luaL_checktype(NLua::luaInterface, -1, LUA_TTABLE);

    QMap<QString, QVariant> list = NLua::convertLuaTable(NLua::luaInterface, -2);
    for(QMap<QString, QVariant>::iterator it=list.begin(); it!=list.end(); ++it)
    {
        driverList[it.key()] = it.value().toString();
        QAction *a = new QAction(it.key(), addDriverButton->menu());
        addDriverButton->menu()->insertAction(addCustomDriverAction, a);
        a->setToolTip(it.value().toString());
    }

    defaultDrivers = NLua::getStringList(NLua::luaInterface, -1);

    lua_pop(NLua::luaInterface, 2);
}

QAction *CNewProjectSettingsPage::getAddAction(const QString &driver)
{
    QList<QAction *> actions = addDriverButton->menu()->actions();
    foreach(QAction *a, actions)
    {
        if (a->text() == driver)
            return a;
    }

    return 0;
}

void CNewProjectSettingsPage::addDriver(QAction *action)
{
    // Special case: add custom driver
    if (action == addCustomDriverAction)
    {
        QString file = QFileDialog::getOpenFileName(this, "Open custom driver",
                                                    QDir::homePath(),
                                                    "driver files (*.lua)");
        if (!file.isEmpty())
        {
            lua_getglobal(NLua::luaInterface, "getDriver");
            lua_pushstring(NLua::luaInterface, qPrintable(file));

            lua_call(NLua::luaInterface, 1, 2);

            if (lua_isnil(NLua::luaInterface, -1))
                QMessageBox::critical(this, "Custom driver error",
                                      "Could not load custom driver (see error log");
            else
            {
                const char *name = luaL_checkstring(NLua::luaInterface, -2);
                const char *desc = luaL_checkstring(NLua::luaInterface, -1);
                QTreeWidgetItem *item = new QTreeWidgetItem(driverTreeWidget,
                                                            QStringList() << name << desc);
                item->setData(0, Qt::UserRole, file);
            }

            lua_pop(NLua::luaInterface, 2);
        }
    }
    else
    {
        action->setVisible(false);
        QStringList l = QStringList() << action->text() <<
                                         driverList[action->text()];
        new QTreeWidgetItem(driverTreeWidget, l);
    }

    if (!delDriverButton->isEnabled())
        delDriverButton->setEnabled(true);
}

void CNewProjectSettingsPage::delDriver()
{
    QTreeWidgetItem *item = driverTreeWidget->currentItem();
    Q_ASSERT(item);

    QAction *a = getAddAction(item->text(0));
    Q_ASSERT(a || !item->data(0, Qt::UserRole).isNull());
    if (a)
        a->setVisible(true);

    delete item;
    if (!driverTreeWidget->topLevelItemCount())
        delDriverButton->setEnabled(false);
}

void CNewProjectSettingsPage::resetDriverTree()
{
    driverTreeWidget->clear();

    for (QMap<QString, QString>::iterator it=driverList.begin();
         it != driverList.end(); ++it)
    {
        bool def = defaultDrivers.contains(it.key());
        if (def)
        {
            QStringList l = QStringList() << it.key() << it.value();
            new QTreeWidgetItem(driverTreeWidget, l);
        }

        QAction *a = getAddAction(it.key());
        Q_ASSERT(a);
        a->setVisible(!def);
    }

    if (!delDriverButton->isEnabled())
        delDriverButton->setEnabled(true);
}

void CNewProjectSettingsPage::initializePage()
{
    setField("RP6Plugin", QDir::homePath());

    if (initDriverList)
    {
        getDriverList();
        initDriverList = false;
    }

    resetDriverTree();
}

bool CNewProjectSettingsPage::isComplete() const
{
    return (QWizardPage::isComplete() &&
            QFileInfo(field("RP6Plugin").toString()).isFile());
}

bool CNewProjectSettingsPage::validatePage()
{
    QString file(field("RP6Plugin").toString());
    return verifyPluginFile(file);
}

QStringList CNewProjectSettingsPage::getSelectedDrivers() const
{
    const int count = driverTreeWidget->topLevelItemCount();
    QStringList ret;
    for (int i=0; i<count; ++i)
    {
        // Custom driver?
        QTreeWidgetItem *item = driverTreeWidget->topLevelItem(i);
        QVariant d = item->data(0, Qt::UserRole);
        if (!d.isNull())
            ret << d.toString();
        else
            ret << item->text(0);
    }

    return ret;
}
