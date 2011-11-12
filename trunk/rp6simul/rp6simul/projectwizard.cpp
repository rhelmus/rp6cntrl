#include "projectwizard.h"
#include "lua.h"
#include "pathinput.h"
#include "projectsettings.h"
#include "rp6simul.h"
#include "utils.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QtGui>

namespace {

QStringList getDefaultDriverList(const TDriverInfoList &driverlist)
{
    QStringList ret;
    for (TDriverInfoList::const_iterator it=driverlist.begin();
         it!=driverlist.end(); ++it)
    {
        if (it->isDefault)
            ret << it->name;
    }

    return ret;
}

}

CProjectWizard::CProjectWizard(QWidget *parent) : QWizard(parent)
{
    setDefaultProperty("CPathInput", "path",
                       SIGNAL(pathTextChanged(const QString &)));
    setModal(true);
    setPage(PAGE_PROJDEST, new CNewProjectDestPage);
    setPage(PAGE_SIMULATOR, projectSimulatorPage = new CNewProjectSimulatorPage);
    setPage(PAGE_ROBOT, new CNewProjectRobotPage);
    setPage(PAGE_M32, new CNewProjectM32Page);
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
        prsettings.setValue("simulator", projectSimulatorPage->getSimulator());

        prsettings.beginGroup("robot");
        prsettings.setValue("plugin", field("robot.plugin"));
        prsettings.setValue("drivers",
                            getDefaultDriverList(CRP6Simulator::getInstance()->getRobotDriverInfoList()));
        prsettings.endGroup();

        prsettings.beginGroup("m32");
        prsettings.setValue("plugin", field("m32.plugin"));
        prsettings.setValue("slot", field("m32.slot"));
        prsettings.setValue("drivers",
                            getDefaultDriverList(CRP6Simulator::getInstance()->getM32DriverInfoList()));
        prsettings.endGroup();

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
    form->setFormAlignment(Qt::AlignLeft | Qt::/*AlignVCenter*/AlignTop);

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
                             "the specified directory. Overwrite?",
                             QMessageBox::Yes | QMessageBox::No);
        return (button == QMessageBox::Yes);
    }


    return true;
}


CNewProjectSimulatorPage::CNewProjectSimulatorPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("Simulator(s) selection");
    setSubTitle("Please select the simulator(s) you want to use. "
                "You can this change later in the project settings dialog.");

    QVBoxLayout *vbox = new QVBoxLayout(this);

    simulatorButtonGroup = new QButtonGroup(this);
    QRadioButton *rb = new QRadioButton("RP6");
    rb->setChecked(true);
    vbox->addWidget(rb);
    simulatorButtonGroup->addButton(rb, SIMULATOR_ROBOT);
    vbox->addWidget(rb = new QRadioButton("m32"));
    simulatorButtonGroup->addButton(rb, SIMULATOR_M32);
    vbox->addWidget(rb = new QRadioButton("RP6 with m32"));
    simulatorButtonGroup->addButton(rb, SIMULATOR_ROBOTM32);
    registerField("robotAndM32", rb);
}

int CNewProjectSimulatorPage::nextId() const
{
    if ((simulatorButtonGroup->checkedId() == SIMULATOR_ROBOT) ||
        (simulatorButtonGroup->checkedId() == SIMULATOR_ROBOTM32))
        return CProjectWizard::PAGE_ROBOT;

    return CProjectWizard::PAGE_M32;
}

ESimulator CNewProjectSimulatorPage::getSimulator() const
{
    return static_cast<ESimulator>(simulatorButtonGroup->checkedId());
}


CNewProjectRobotPage::CNewProjectRobotPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("RP6 settings");
    setSubTitle("Please fill in the RP6 settings below.\n"
                "NOTE: See the manual for details on how to create plugin files.");

    QFormLayout *form = new QFormLayout(this);

    CPathInput *pathinput = new CPathInput("Open RP6 plugin file",
                                           CPathInput::PATH_EXISTFILE,
                                           QDir::homePath(), getPluginFilter());
    form->addRow("RP6 plugin", pathinput);
    registerField("robot.plugin", pathinput);
    connect(pathinput, SIGNAL(pathTextChanged(const QString &)),
            SIGNAL(completeChanged()));
}

bool CNewProjectRobotPage::isComplete() const
{
    return (QWizardPage::isComplete() &&
            QFileInfo(field("robot.plugin").toString()).isFile());
}

int CNewProjectRobotPage::nextId() const
{
    if (field("robotAndM32").toBool())
        return CProjectWizard::PAGE_M32;
    return -1;
}

bool CNewProjectRobotPage::validatePage()
{
    const QString file(field("robot.plugin").toString());
    return verifyPluginFile(file);
}


CNewProjectM32Page::CNewProjectM32Page(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("m32 settings");
    setSubTitle("Please fill in the m32 settings below.\n"
                "NOTE: See the manual for details on how to create plugin files.");

    QFormLayout *form = new QFormLayout(this);

    CPathInput *pathinput = new CPathInput("Open RP6 plugin file",
                                           CPathInput::PATH_EXISTFILE,
                                           QDir::homePath(), getPluginFilter());
    form->addRow("m32 plugin", pathinput);
    registerField("m32.plugin", pathinput);
    connect(pathinput, SIGNAL(pathTextChanged(const QString &)),
            SIGNAL(completeChanged()));

    form->addRow("m32 position", slotComboBox = new QComboBox);
    slotComboBox->addItems(QStringList() << "Front" << "Back");
    slotComboBox->setCurrentIndex(1);
    registerField("m32.slot", slotComboBox);
    formSlotLabel = form->labelForField(slotComboBox);
}

void CNewProjectM32Page::initializePage()
{
    const bool e = field("robotAndM32").toBool();
    slotComboBox->setVisible(e);
    formSlotLabel->setVisible(e);
}

bool CNewProjectM32Page::isComplete() const
{
    return (QWizardPage::isComplete() &&
            QFileInfo(field("m32.plugin").toString()).isFile());
}

bool CNewProjectM32Page::validatePage()
{
    const QString file(field("m32.plugin").toString());
    return verifyPluginFile(file);
}
