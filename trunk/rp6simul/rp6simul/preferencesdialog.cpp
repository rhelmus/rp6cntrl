#include "pathinput.h"
#include "preferencesdialog.h"
#include "simulator.h"

#include <QtGui>

#include <qextserialenumerator.h>

namespace {

bool stringIntLessThan(const QString &left, const QString &right)
{
    return left.toInt() < right.toInt();
}

void setComboBoxCurrentString(QComboBox *combo, const QString &str)
{
    const int index = combo->findText(str);
    if (index != -1)
        combo->setCurrentIndex(index);
}

}

CPreferencesDialog::CPreferencesDialog(QWidget *parent) : QDialog(parent)
{
    setModal(true);

    QVBoxLayout *vbox = new QVBoxLayout(this);

    QTabWidget *tabw = new QTabWidget;
    vbox->addWidget(tabw);

    tabw->addTab(createGeneralTab(), "General");
    tabw->addTab(createSoundTab(), "Sound");
    tabw->addTab(createSerialTab(), "Serial");
    tabw->addTab(createSimulationTab(), "Simulation");

    QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                  QDialogButtonBox::Cancel);
    connect(bbox, SIGNAL(accepted()), SLOT(accept()));
    connect(bbox, SIGNAL(rejected()), SLOT(reject()));
    vbox->addWidget(bbox);
}

QWidget *CPreferencesDialog::createGeneralTab()
{
    QWidget *ret = new QWidget;
    QFormLayout *form = new QFormLayout(ret);

    defaultProjectDirPathInput = new CPathInput("Default project directory",
                                                CPathInput::PATH_EXISTDIR);
    form->addRow("Default project directory", defaultProjectDirPathInput);

    form->addRow("Load last project on program start",
                 loadPrevProjectCheckBox = new QCheckBox);

    return ret;
}

QWidget *CPreferencesDialog::createSoundTab()
{
    QWidget *ret = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout(ret);

    vbox->addWidget(soundGroupBox = new QGroupBox("Sound"), 0, Qt::AlignTop);
    soundGroupBox->setCheckable(true);
    soundGroupBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QFormLayout *form = new QFormLayout(soundGroupBox);

    form->addRow("Piezo volume", piezoVolumeSlider = new QSlider(Qt::Horizontal));
    piezoVolumeSlider->setRange(0, 100);

    return ret;
}

QWidget *CPreferencesDialog::createSerialTab()
{
    QWidget *ret = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout(ret);

    QLabel *l = new QLabel("Here you can link a virtual or physical "
                           "virtual port to the RP6 or m32 serial lines. "
                           "Most often you only want to change the serial port. "
                           "See the manual for more details.");
    l->setWordWrap(true);
    l->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    vbox->addWidget(l);

    vbox->addWidget(robotSerialGroupBox = new QGroupBox("RP6"));
    robotSerialGroupBox->setCheckable(true);

    QVBoxLayout *subvbox = new QVBoxLayout(robotSerialGroupBox);
    subvbox->addWidget(robotSerialWidget = new CSerialPreferencesWidget);

    vbox->addWidget(m32SerialGroupBox = new QGroupBox("m32"));
    m32SerialGroupBox->setCheckable(true);
    subvbox = new QVBoxLayout(m32SerialGroupBox);
    subvbox->addWidget(m32SerialWidget = new CSerialPreferencesWidget);

    return ret;
}

QWidget *CPreferencesDialog::createSimulationTab()
{
    QWidget *ret = new QWidget;

    QVBoxLayout *vbox = new QVBoxLayout(ret);

    QGroupBox *group = new QGroupBox("CPU usage");
    vbox->addWidget(group, 0, Qt::AlignTop);
    QVBoxLayout *subvbox = new QVBoxLayout(group);

    QLabel *l = new QLabel("Here you can roughly select the amount of CPU cycles "
                           "the simulator(s) will consume. In general you want "
                           "to keep this at 'normal'. However if you notice that "
                           "the simulated frequency is below the normal value "
                           "(i.e. below 8 or 16 MHz) than it may be useful to "
                           "change the setting to 'max'. If you want to save "
                           "some CPU cycles you may choose the 'low' setting, "
                           "but be careful that simulation frequencies stay OK!");
    l->setWordWrap(true);
    l->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    subvbox->addWidget(l);

    subvbox->addWidget(simulationCPUComboBox = new QComboBox);
    simulationCPUComboBox->addItems(QStringList() << "low" << "normal" << "max");
    simulationCPUComboBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    return ret;
}

void CPreferencesDialog::loadPreferences(QSettings &settings)
{
    /* Preferences
      - Default project directory
      - Sound enabled
      - Piezo volume
      - Serial ports for robot and m32 pass-through + their settings
      - Simulation CPU usage
    */

    defaultProjectDirPathInput->setPath(settings.value("defaultProjectDir",
                                                       QDir::homePath()).toString());
    loadPrevProjectCheckBox->setChecked(settings.value("loadPrevProjectStartup", true).toBool());

    soundGroupBox->setChecked(settings.value("soundEnabled", true).toBool());
    piezoVolumeSlider->setValue(settings.value("piezoVolume", 75).toInt());

    settings.beginGroup("robot");
    robotSerialGroupBox->setChecked(settings.value("enabled", false).toBool());
    robotSerialWidget->loadPreferences(settings);
    settings.endGroup();

    settings.beginGroup("m32");
    m32SerialGroupBox->setChecked(settings.value("enabled", false).toBool());
    m32SerialWidget->loadPreferences(settings);
    settings.endGroup();

    const ESimulatorCPUUsage cpu =
            static_cast<ESimulatorCPUUsage>(settings.value("simulationCPUUsage", CPU_NORMAL).toInt());
    simulationCPUComboBox->setCurrentIndex(cpu);
}

void CPreferencesDialog::savePreferences(QSettings &settings)
{
    settings.setValue("defaultProjectDir", defaultProjectDirPathInput->getPath());
    settings.setValue("loadPrevProjectStartup",
                      loadPrevProjectCheckBox->isChecked());

    settings.setValue("soundEnabled", soundGroupBox->isChecked());
    settings.setValue("piezoVolume", piezoVolumeSlider->value());

    settings.beginGroup("robot");
    settings.setValue("enabled", robotSerialGroupBox->isChecked());
    robotSerialWidget->savePreferences(settings);
    settings.endGroup();

    settings.beginGroup("m32");
    settings.setValue("enabled", m32SerialGroupBox->isChecked());
    m32SerialWidget->savePreferences(settings);
    settings.endGroup();

    settings.setValue("simulationCPUUsage", simulationCPUComboBox->currentIndex());
}

void CPreferencesDialog::accept()
{
    bool showmsg = false;
    QString sim;

    if (robotSerialGroupBox->isChecked() &&
        robotSerialWidget->getPort().isEmpty())
    {
        showmsg = true;
        sim = "RP6";
    }
    else if (m32SerialGroupBox->isChecked() &&
             m32SerialWidget->getPort().isEmpty())
    {
        showmsg = true;
        sim = "m32";
    }

    if (showmsg)
    {
        const QString msg = QString("No %1 serial port specified. Either "
                                    "disable this option or specify a port.").arg(sim);
        QMessageBox::critical(this, "No serial port specified", msg);
        return;
    }

    QDialog::accept();
}


CSerialPreferencesWidget::CSerialPreferencesWidget(QWidget *parent,
                                                   Qt::WindowFlags f)
    : QWidget(parent, f)
{
    QFormLayout *form = new QFormLayout(this);

    QWidget *w = new QWidget;
    QHBoxLayout *hbox = new QHBoxLayout(w);
    hbox->addWidget(portEdit = new QLineEdit);
    QPushButton *button = new QPushButton("ports...");
    connect(button, SIGNAL(clicked()), SLOT(showSerialPortSelection()));
    hbox->addWidget(button);
    form->addRow("Serial port (e.g. COMx or /dev/ttyX)", w);

    form->addRow("Baud rate", baudRateComboBox = new QComboBox);
    QStringList slist = QStringList() << "110" << "300" << "600" <<
                                         "1200" << "2400" << "4800" << "9600" <<
                                         "19200" << "38400" << "57600" << "115200";
    // Baud rates valid for both POSIX and Windows (according to qextserialport.h)

#if Q_OS_WIN
    slist << "14400" << "56000" << "128000" << "256000";
#else
    slist << "50" << "75" << "134" << "150" << "200" << "1800" << "76800";
#endif
    qSort(slist.begin(), slist.end(), stringIntLessThan);
    baudRateComboBox->addItems(slist);

    form->addRow("Parity", parityComboBox = new QComboBox);
    slist = QStringList() << "none" << "odd" << "even" << "space";
#if Q_OS_WIN
    slist << "mark";
#endif
    parityComboBox->addItems(slist);

    form->addRow("Data bits", dataBitsComboBox = new QComboBox);
    dataBitsComboBox->addItems(QStringList() << "5" << "6" << "7" << "8");

    form->addRow("Stop bits", stopBitsComboBox = new QComboBox);
    slist = QStringList() << "1" << "2";
#ifdef Q_OS_WIN
    slist << "1.5";
#endif
    stopBitsComboBox->addItems(slist);
}

void CSerialPreferencesWidget::showSerialPortSelection()
{
    QDialog dialog(this);

    QVBoxLayout *vbox = new QVBoxLayout(&dialog);

    QTreeWidget *tree = new QTreeWidget;
    tree->setHeaderLabels(QStringList() << "Port" << "Descriptive name");
    tree->setRootIsDecorated(false);
    vbox->addWidget(tree);

    QList<QextPortInfo> ports = QextSerialEnumerator::getPorts();
    const int size = ports.size();
    for (int i=0; i<size; ++i)
    {
        new QTreeWidgetItem(tree, QStringList() << ports.at(i).physName <<
                            ports.at(i).friendName);
    }

    QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                  QDialogButtonBox::Cancel);
    connect(bbox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(bbox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    vbox->addWidget(bbox);

    if (dialog.exec() == QDialog::Accepted)
    {
        if (tree->currentItem() && !tree->currentItem()->text(0).isEmpty())
            portEdit->setText(tree->currentItem()->text(0));
    }
}

void CSerialPreferencesWidget::loadPreferences(QSettings &settings)
{
    /* Serial settings
      - Port
      - Baud rate
      - Parity
      - Data bits
      - Stop bits
    */

    portEdit->setText(settings.value("port").toString());
    setComboBoxCurrentString(baudRateComboBox,
                             settings.value("baudRate", "38400").toString());
    setComboBoxCurrentString(parityComboBox,
                             settings.value("parity", "none").toString());
    setComboBoxCurrentString(dataBitsComboBox,
                             settings.value("dataBits", "8").toString());
    setComboBoxCurrentString(stopBitsComboBox,
                             settings.value("stopBits", "1").toString());
}

void CSerialPreferencesWidget::savePreferences(QSettings &settings)
{
    settings.setValue("port", portEdit->text());
    settings.setValue("baudRate", baudRateComboBox->currentText());
    settings.setValue("parity", parityComboBox->currentText());
    settings.setValue("dataBits", dataBitsComboBox->currentText());
    settings.setValue("stopBits", stopBitsComboBox->currentText());
}

QString CSerialPreferencesWidget::getPort() const
{
    return portEdit->text();
}
