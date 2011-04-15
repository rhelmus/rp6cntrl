#include "projectwizard.h"
#include "pathinput.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QtGui>

CProjectWizard::CProjectWizard(QWidget *parent) : QWizard(parent)
{
    setDefaultProperty("CPathInput", "path",
                       SIGNAL(pathTextChanged(const QString &)));
    addPage(new CNewProjectDestPage);
    addPage(new CNewProjectSettingsPage);
}

void CProjectWizard::accept()
{
    qDebug() << "Accepted:";
    QDialog::accept();
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
        QMessageBox::warning(this, "Project directory error",
                             "You do not have sufficient permissions for "
                             "the directory specified.");
        return false;
    }

    // UNDONE: Check file also

    return true;
}


CNewProjectSettingsPage::CNewProjectSettingsPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("Project settings");

    QGridLayout *grid = new QGridLayout(this);

    QGroupBox *group = new QGroupBox("RP6 plugin");
    grid->addWidget(group, 0, 0);
    QVBoxLayout *vbox = new QVBoxLayout(group);
    CPathInput *pathinput = new CPathInput("Open RP6 plugin file",
                                           CPathInput::PATH_EXISTFILE);
    vbox->addWidget(pathinput);
    registerField("RP6Plugin", pathinput);
    connect(pathinput, SIGNAL(pathTextChanged(const QString &)), this,
            SIGNAL(completeChanged()));

    group = new QGroupBox("m32 plugin");
    grid->addWidget(group, 0, 1);
    vbox = new QVBoxLayout(group);
    pathinput = new CPathInput("Open m32 plugin file",
                               CPathInput::PATH_EXISTFILE);
    vbox->addWidget(pathinput);
    registerField("m32Plugin", pathinput);
    connect(pathinput, SIGNAL(pathTextChanged(const QString &)), this,
            SIGNAL(completeChanged()));

    group = new QGroupBox("RP6 plugin");
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

bool CNewProjectSettingsPage::checkPermissions(const QString &file) const
{
    // UNDONE: Need exec?
    QFileInfo fi(file);
    return (fi.isReadable());
}

void CNewProjectSettingsPage::initializePage()
{
    setField("RP6Plugin", QDir::homePath());
    setField("m32Plugin", QDir::homePath());
}

bool CNewProjectSettingsPage::isComplete() const
{
    bool ret = QWizardPage::isComplete();

    if (ret)
        ret = (QFileInfo(field("RP6Plugin").toString()).isFile() ||
               QFileInfo(field("m32Plugin").toString()).isFile());

    return ret;
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

    file = field("m32Plugin").toString();
    if (!file.isEmpty() && !checkPermissions(file))
    {
        QMessageBox::warning(this, "m32 plugin error",
                             "You do not have sufficient permissions for "
                             "the RP6 plugin specified.");
        return false;
    }

    return true;
}
