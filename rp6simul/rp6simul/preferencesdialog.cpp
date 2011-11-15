#include "pathinput.h"
#include "preferencesdialog.h"

#include <QtGui>

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

    form->addRow("Default project directory",
                 new CPathInput("Default project directory",
                                CPathInput::PATH_EXISTDIR));

    return ret;
}

QWidget *CPreferencesDialog::createSoundTab()
{
    QWidget *ret = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout(ret);

    QGroupBox *group = new QGroupBox("Sound");
    group->setCheckable(true);
    group->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    vbox->addWidget(group, 0, Qt::AlignTop);
    QFormLayout *form = new QFormLayout(group);

    form->addRow("Piezo volume", new QSlider(Qt::Horizontal));

    return ret;
}

QWidget *CPreferencesDialog::createSerialTab()
{
    QWidget *ret = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout(ret);

    QLabel *l = new QLabel("In this creen you can link a virtual or physical "
                           "virtual port to the RP6 or m32 serial lines. "
                           "Most often you only want to change the serial port. "
                           "See the manual for more details.");
    l->setWordWrap(true);
    l->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    vbox->addWidget(l);

    QGroupBox *group = new QGroupBox("RP6");
    group->setCheckable(true);
    vbox->addWidget(group);
    QVBoxLayout *subvbox = new QVBoxLayout(group);
    subvbox->addWidget(new CSerialPreferencesWidget);

    vbox->addWidget(group = new QGroupBox("m32"));
    group->setCheckable(true);
    subvbox = new QVBoxLayout(group);
    subvbox->addWidget(new CSerialPreferencesWidget);

    return ret;
}

QWidget *CPreferencesDialog::createSimulationTab()
{
    QWidget *ret = new QWidget;

    QVBoxLayout *vbox = new QVBoxLayout(ret);

    QGroupBox *group = new QGroupBox("Sound");
    group->setCheckable(true);
    group->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    vbox->addWidget(group, 0, Qt::AlignTop);
    QFormLayout *form = new QFormLayout(group);

    // UNDONE: Label
    form->addRow("CPU usage", new QComboBox);

    return ret;
}


CSerialPreferencesWidget::CSerialPreferencesWidget(QWidget *parent,
                                                   Qt::WindowFlags f)
    : QWidget(parent, f)
{
    QFormLayout *form = new QFormLayout(this);

    // UNDONE: List serial ports?
    form->addRow("Serial port (e.g. COMx or /dev/ttyX)", new QLineEdit);
    form->addRow("Baud rate", new QComboBox);
    form->addRow("Parity", new QComboBox);
    form->addRow("Data bits", new QComboBox);
    form->addRow("Stop bits", new QComboBox);
}
