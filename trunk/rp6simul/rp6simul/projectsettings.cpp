#include "pathinput.h"
#include "projectsettings.h"
#include "rp6simul.h"

#include <QLibrary>
#include <QtGui>

CProjectSettingsDialog::CProjectSettingsDialog(QWidget *parent, Qt::WindowFlags f)
    : QDialog(parent, f)
{
    setModal(true);

    QVBoxLayout *vbox = new QVBoxLayout(this);

    QTabWidget *tabw = new QTabWidget;
    vbox->addWidget(tabw);

    tabw->addTab(createMainTab(), "Main");
    tabw->addTab(robotTab = createRobotTab(), "RP6");
    tabw->addTab(m32Tab = createM32Tab(), "m32");

    QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                  QDialogButtonBox::Cancel);
    connect(bbox, SIGNAL(accepted()), SLOT(accept()));
    connect(bbox, SIGNAL(rejected()), SLOT(reject()));
    vbox->addWidget(bbox);
}

QWidget *CProjectSettingsDialog::createMainTab()
{
    QWidget *ret = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout(ret);

    QGroupBox *group = new QGroupBox("Simulator(s)");
    group->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    vbox->addWidget(group, 0, Qt::AlignTop);
    QVBoxLayout *subvbox = new QVBoxLayout(group);

    simulatorButtonGroup = new QButtonGroup;
    connect(simulatorButtonGroup, SIGNAL(buttonClicked(int)),
            SLOT(updateSimulatorSelection(int)));
    QRadioButton *rb = new QRadioButton("RP6");
    subvbox->addWidget(rb);
    simulatorButtonGroup->addButton(rb, SIMULATOR_ROBOT);
    subvbox->addWidget(rb = new QRadioButton("m32"));
    simulatorButtonGroup->addButton(rb, SIMULATOR_M32);
    subvbox->addWidget(rb = new QRadioButton("RP6 with m32"));
    simulatorButtonGroup->addButton(rb, SIMULATOR_ROBOTM32);

    return ret;
}

QWidget *CProjectSettingsDialog::createRobotTab()
{
    QWidget *ret = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout(ret);

    QGroupBox *group = new QGroupBox("RP6 plugin");
    group->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    vbox->addWidget(group);
    QVBoxLayout *subvbox = new QVBoxLayout(group);

    robotPluginInput = new CPathInput("Open RP6 plugin file",
                                      CPathInput::PATH_EXISTFILE, QString(),
                                      getPluginFilter());
    subvbox->addWidget(robotPluginInput);


    vbox->addWidget(group = new QGroupBox("Drivers"));
    subvbox = new QVBoxLayout(group);

    robotDriverWidget =
            new CDriverSelectionWidget(CRP6Simulator::getInstance()->getRobotDriverInfoList());
    subvbox->addWidget(robotDriverWidget);

    return ret;
}

QWidget *CProjectSettingsDialog::createM32Tab()
{
    QWidget *ret = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout(ret);

    QGroupBox *group = new QGroupBox("m32 plugin");
    group->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    vbox->addWidget(group);
    QVBoxLayout *subvbox = new QVBoxLayout(group);

    m32PluginInput = new CPathInput("Open m32 plugin file",
                                    CPathInput::PATH_EXISTFILE, QString(),
                                    getPluginFilter());
    subvbox->addWidget(m32PluginInput);


    vbox->addWidget(group = new QGroupBox("m32 position"));
    group->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    subvbox = new QVBoxLayout(group);

    subvbox->addWidget(m32SlotComboBox = new QComboBox);
    m32SlotComboBox->addItems(QStringList() << "Front" << "Back");


    vbox->addWidget(group = new QGroupBox("Drivers"));
    subvbox = new QVBoxLayout(group);

    m32DriverWidget =
            new CDriverSelectionWidget(CRP6Simulator::getInstance()->getM32DriverInfoList());
    subvbox->addWidget(m32DriverWidget);

    return ret;
}

void CProjectSettingsDialog::updateSimulatorSelection(int simulator)
{
    robotTab->setEnabled((simulator == SIMULATOR_ROBOT) ||
                         (simulator == SIMULATOR_ROBOTM32));
    m32Tab->setEnabled((simulator == SIMULATOR_M32) ||
                       (simulator == SIMULATOR_ROBOTM32));
    m32SlotComboBox->setEnabled(simulator == SIMULATOR_ROBOTM32);
}

void CProjectSettingsDialog::loadSettings(CProjectSettings &settings)
{
    /* Settings
      - Simulators
      - Robot plugin file, drivers
      - m32 plugin file, slot, drivers
    */

    const ESimulator simulator =
            static_cast<ESimulator>(settings.value("simulator", SIMULATOR_ROBOT).toInt());
    simulatorButtonGroup->button(simulator)->setChecked(true);
    updateSimulatorSelection(simulator);

    settings.beginGroup("robot");
    robotPluginInput->setPath(settings.value("plugin").toString());
    robotDriverWidget->selectDrivers(settings.value("drivers").toStringList());
    settings.endGroup();

    settings.beginGroup("m32");
    m32PluginInput->setPath(settings.value("plugin").toString());
    const EM32Slot slot =
            static_cast<EM32Slot>(settings.value("slot", SLOT_BACK).toInt());
    m32SlotComboBox->setCurrentIndex((slot == SLOT_BACK) ? 1 : 0);
    m32DriverWidget->selectDrivers(settings.value("drivers").toStringList());
    settings.endGroup();
}

void CProjectSettingsDialog::saveSettings(CProjectSettings &settings)
{
    settings.setValue("simulator", simulatorButtonGroup->checkedId());

    settings.beginGroup("robot");
    settings.setValue("plugin", robotPluginInput->getPath());
    settings.setValue("drivers", robotDriverWidget->getSelectedDrivers());
    settings.endGroup();

    settings.beginGroup("m32");
    settings.setValue("plugin", m32PluginInput->getPath());
    settings.setValue("slot", (m32SlotComboBox->currentIndex() == 1) ? SLOT_BACK : SLOT_FRONT);
    settings.setValue("drivers", m32DriverWidget->getSelectedDrivers());
    settings.endGroup();

    settings.sync(); // Force commit
}

void CProjectSettingsDialog::accept()
{
    // Verify settings
    if (!verifyPluginFile(robotPluginInput->getPath()) ||
        !verifyPluginFile(m32PluginInput->getPath()))
        return;

    QDialog::accept();
}

CDriverSelectionWidget::CDriverSelectionWidget(const TDriverInfoList &drlist,
                                               QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f), driverInfoList(drlist)
{
    // UNDONE: Description?

    QHBoxLayout *hbox = new QHBoxLayout(this);

    hbox->addWidget(driverTreeWidget = new QTreeWidget);
    driverTreeWidget->setHeaderLabels(QStringList() << "Driver" << "Description");
    driverTreeWidget->setRootIsDecorated(false);

    QVBoxLayout *vbox = new QVBoxLayout;
    hbox->addLayout(vbox);

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

    initDrivers();
}

void CDriverSelectionWidget::initDrivers()
{
    for(TDriverInfoList::iterator it=driverInfoList.begin();
        it!=driverInfoList.end(); ++it)
    {
        QAction *a = new QAction(it->name, addDriverButton->menu());
        a->setData(it->description);
        addDriverButton->menu()->insertAction(addCustomDriverAction, a);
        a->setToolTip(it->description);
    }

    resetDriverTree();
}

QAction *CDriverSelectionWidget::getAddAction(const QString &driver)
{
    QList<QAction *> actions = addDriverButton->menu()->actions();
    foreach(QAction *a, actions)
    {
        if (a->text() == driver)
            return a;
    }

    return 0;
}

void CDriverSelectionWidget::addCustomDriver(const QString &file)
{
    QString name, desc;
    const bool ok = CRP6Simulator::getInstance()->loadCustomDriverInfo(file,
                                                                       name,
                                                                       desc);
    if (!ok)
        QMessageBox::critical(this, "Custom driver error",
                              "Could not load custom driver (see error log");
    else
    {
        QTreeWidgetItem *item =
                new QTreeWidgetItem(driverTreeWidget,
                                    QStringList() << name << desc);
        item->setData(0, Qt::UserRole, file);
    }
}

void CDriverSelectionWidget::addDriver(QAction *action)
{
    // Special case: add custom driver
    if (action == addCustomDriverAction)
    {
        QString file = QFileDialog::getOpenFileName(this, "Open custom driver",
                                                    QDir::homePath(),
                                                    "driver files (*.lua)");
        if (!file.isEmpty())
            addCustomDriver(file);
    }
    else
    {
        action->setVisible(false);
        QStringList l = QStringList() << action->text() <<
                                         action->data().toString();
        new QTreeWidgetItem(driverTreeWidget, l);
    }

    if (!delDriverButton->isEnabled())
        delDriverButton->setEnabled(true);
}

void CDriverSelectionWidget::delDriver()
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

void CDriverSelectionWidget::resetDriverTree()
{
    driverTreeWidget->clear();

    for (TDriverInfoList::iterator it=driverInfoList.begin();
         it != driverInfoList.end(); ++it)
    {
        if (it->isDefault)
        {
            QStringList l = QStringList() << it->name << it->description;
            new QTreeWidgetItem(driverTreeWidget, l);
        }

        QAction *a = getAddAction(it->name);
        Q_ASSERT(a);
        a->setVisible(!it->isDefault);
    }

    if (driverTreeWidget->topLevelItemCount())
    {
        driverTreeWidget->resizeColumnToContents(0);
        driverTreeWidget->resizeColumnToContents(1);
        if (!delDriverButton->isEnabled())
            delDriverButton->setEnabled(true);
    }
}

void CDriverSelectionWidget::selectDrivers(QStringList drivers)
{
    driverTreeWidget->clear();

    for (TDriverInfoList::iterator it=driverInfoList.begin();
         it!=driverInfoList.end(); ++it)
    {
        QAction *a = getAddAction(it->name);
        Q_ASSERT(a);

        if (drivers.contains(it->name))
        {
            QStringList l = QStringList() << it->name << it->description;
            new QTreeWidgetItem(driverTreeWidget, l);
            a->setVisible(false);
            drivers.removeAll(it->name);
        }
        else
            a->setVisible(true);
    }

    if (!drivers.isEmpty()) // Custom drivers?
    {
        foreach (QString d, drivers)
        {
            if (!d.endsWith(".lua"))
            {
                qWarning() << "Found custom driver without lua extension:" << d;
                continue;
            }
            addCustomDriver(d);
        }
    }

    if (driverTreeWidget->topLevelItemCount())
    {
        driverTreeWidget->resizeColumnToContents(0);
        driverTreeWidget->resizeColumnToContents(1);
        if (!delDriverButton->isEnabled())
            delDriverButton->setEnabled(true);
    }
}

QStringList CDriverSelectionWidget::getSelectedDrivers() const
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


bool verifyPluginFile(const QString &file)
{
    QString msg;
    if (file.isEmpty())
        msg = "No plugin file specified.";
    else if (!QFileInfo(file).isReadable())
        msg = "You do not have sufficient permissions for "
              "the RP6 plugin specified.";
    else if (!QLibrary::isLibrary(file))
        msg = "Incorrect file type selected.";

    if (!msg.isEmpty())
    {
        QMessageBox::warning(CRP6Simulator::getInstance(), "Plugin error", msg);
        return false;
    }

    return true;
}
