#include "projectwizard.h"
#include "lua.h"
#include "pathinput.h"
#include "projectsettings.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
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
    prsettings.sync(); // Used for status, see docs

    if (prsettings.status() == QSettings::AccessError)
    {
        QMessageBox::critical(this, "File access error", "Unable to access project file!");
        reject();
    }
    else if (prsettings.status() == QSettings::FormatError)
    {
        // Hmm does this make any sense??
        QMessageBox::critical(this, "File format error", "Invalid file format!");
        reject();
    }
    else
    {
        prsettings.setValue("version", 1);
        prsettings.setValue("name", field("projectName"));
        prsettings.setValue("drivers", projectSettingsPage->getSelectedDrivers());
        QDialog::accept();
    }
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
    vbox->addWidget(new QPushButton("Add driver"));
    vbox->addWidget(new QPushButton("Remove driver"));
    vbox->addWidget(new QPushButton("Reset"));
}

void CNewProjectSettingsPage::getDriverList()
{
    lua_getglobal(NLua::luaInterface, "getDriverList");
    lua_call(NLua::luaInterface, 0, 1);

    QMap<QString, QVariant> list = NLua::convertLuaTable(NLua::luaInterface, -1);
    for(QMap<QString, QVariant>::iterator it=list.begin(); it!=list.end(); ++it)
    {
        driverList[it.key()] = it.value().toString();
    }

    lua_pop(NLua::luaInterface, 1);
}

bool CNewProjectSettingsPage::checkPermissions(const QString &file) const
{
    // UNDONE: Need exec?
    QFileInfo fi(file);
    return (fi.isReadable());
}

void CNewProjectSettingsPage::resetDriverTree()
{
    driverTreeWidget->clear();

    for (QMap<QString, QString>::iterator it=driverList.begin();
         it!=driverList.end(); ++it)
    {
        QStringList l = QStringList() << it.key() << it.value();
        new QTreeWidgetItem(driverTreeWidget, l);
    }
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
    if (!file.isEmpty() && !checkPermissions(file))
    {
        QMessageBox::warning(this, "RP6 plugin error",
                             "You do not have sufficient permissions for "
                             "the RP6 plugin specified.");
        return false;
    }

    return true;
}

QStringList CNewProjectSettingsPage::getSelectedDrivers() const
{
    const int count = driverTreeWidget->topLevelItemCount();
    QStringList ret;
    for (int i=0; i<count; ++i)
        ret << driverTreeWidget->topLevelItem(i)->text(0);
    return ret;
}
