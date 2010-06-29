#include <stdint.h>

#include <QtGui>
#include <QtNetwork>

#include <qwt_slider.h>

#include "client.h"
#include "editor.h"
#include "navmap.h"
#include "scanner.h"
#include "sensorplot.h"
#include "tcputil.h"

namespace {

QLabel *createDataLabel(const QString &l)
{
    QLabel *ret = new QLabel(l);
    ret->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    return ret;
}

QString directionString(uint8_t dir)
{
    switch (dir)
    {
        case FWD: return "forward";
        case BWD: return "backward";
        case LEFT: return "left";
        case RIGHT: return "right";
    }
    
    return QString();
}

QString ACSStateToString(uint8_t state)
{
    switch (state)
    {
        case ACS_STATE_IDLE: return "idle";
        case ACS_STATE_IRCOMM_DELAY: return "checking for incoming ircomm";
        case ACS_STATE_SEND_LEFT: return "sending left pulses";
        case ACS_STATE_WAIT_LEFT: return "receiving left pulses";
        case ACS_STATE_SEND_RIGHT: return "sending right pulses";
        case ACS_STATE_WAIT_RIGHT: return "receiving right pulses";
    }
    
    return QString();
}

QString ACSPowerToString(EACSPowerState state)
{
    switch (state)
    {
        case ACS_POWER_OFF: return "off";
        case ACS_POWER_LOW: return "low";
        case ACS_POWER_MED: return "med";
        case ACS_POWER_HIGH: return "high";
    }
    
    return QString();
}

QPushButton *createDriveButton(const QIcon &icon, Qt::Key key)
{
    QPushButton *ret = new QPushButton;
    ret->setIcon(icon);
    ret->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ret->setShortcut(key);
    return ret;
}

QWidget *createSlider(const QString &title, QwtSlider *&slider, int min, int max)
{
    QFrame *ret = new QFrame;
    ret->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    ret->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);

    QVBoxLayout *vbox = new QVBoxLayout(ret);

    QLabel *label = new QLabel(title);
    label->setAlignment(Qt::AlignCenter);
    vbox->addWidget(label);

    vbox->addWidget(slider = new QwtSlider(0, Qt::Vertical, QwtSlider::LeftScale));
    slider->setScale(min, max);
    slider->setRange(min, max);
    slider->setReadOnly(true);
    
    return ret;
}

float standardDeviation(const QList<int> &values)
{
    if (values.size() < 2)
        return 1.0;
    
    const int size = values.size();
    int sum = 0;
    
    for (QList<int>::const_iterator it=values.begin(); it!=values.end(); ++it)
        sum += *it;
    
    float mean = static_cast<float>(sum) / size;
    float qsum = 0.0;
    
    for (QList<int>::const_iterator it=values.begin(); it!=values.end(); ++it)
        qsum += pow(*it - mean, 2.0);
    
    return sqrt(qsum / (size-1));
}

}


CQtClient::CQtClient() : isACSScanning(false), alternatingACSScan(false),
                         remainingACSScanCycles(0), isTurretScanning(false), turretScanEndRange(0),
                         turretScanResolution(0), turretScanTime(0), turretScanDelay(0),
                         currentScanPosition(0), previousScriptItem(NULL), firstStateUpdate(false),
                         ACSPowerState(ACS_POWER_OFF)
{
    QTabWidget *mainTab = new QTabWidget;
    mainTab->setTabPosition(QTabWidget::West);
    setCentralWidget(mainTab);
    
    mainTab->addTab(createMainTab(), "Main");
    mainTab->addTab(createLuaTab(), "Lua");
    mainTab->addTab(createNavTab(), "Navigation");
    mainTab->addTab(createConsoleTab(), "Console");
    mainTab->addTab(createLogTab(), "Log");
    
    QTimer *uptimer = new QTimer(this);
    connect(uptimer, SIGNAL(timeout()), this, SLOT(updateSensors()));
    uptimer->start(500);
    
    motorDistance[0] = motorDistance[1] = 0;
    destMotorDistance[0] = destMotorDistance[1] = 0;
    
    turretScanTimer = new QTimer(this);
    connect(turretScanTimer, SIGNAL(timeout()), this, SLOT(turretScanTimeout()));

    simNavTimer = new QTimer(this);
    connect(simNavTimer, SIGNAL(timeout()), this, SLOT(simNavTimeout()));

    updateConnection(false);

    appendLogOutput("Started RP6 Qt frontend.\n");
}

QWidget *CQtClient::createMainTab()
{
    QWidget *ret = new QWidget;
    
    QVBoxLayout *vbox = new QVBoxLayout(ret);
    
    QSplitter *splitter = new QSplitter(Qt::Vertical);
    vbox->addWidget(splitter);
    
    QTabWidget *tabWidget = new QTabWidget;
    splitter->addWidget(tabWidget);
    tabWidget->addTab(createOverviewWidget(), "Overview");
    tabWidget->addTab(createSpeedWidget(), "Motor speed");
    tabWidget->addTab(createDistWidget(), "Motor distance");
    tabWidget->addTab(createCurrentWidget(), "Motor current");
    tabWidget->addTab(createLightWidget(), "Light sensors");
    tabWidget->addTab(createACSWidget(), "ACS");
    tabWidget->addTab(createBatteryWidget(), "Battery");
    tabWidget->addTab(createMicWidget(), "Microphone");
    tabWidget->addTab(createSharpIRWidget(), "Sharp IR");

    splitter->addWidget(tabWidget = new QTabWidget);
    tabWidget->addTab(createConnectionWidget(), "Connection");
    tabWidget->addTab(createDriveWidget(), "Drive");
    tabWidget->addTab(createScannerWidget(), "Scan");
    tabWidget->addTab(createIRTurretWidget(), "IR turret");
    
    return ret;
}

QWidget *CQtClient::createLuaTab()
{
    QSplitter *split = new QSplitter(Qt::Vertical);
    
    QGroupBox *group = new QGroupBox("Editor");
    split->addWidget(group);
    QVBoxLayout *vbox = new QVBoxLayout(group);
    
    vbox->addWidget(scriptEditor = new CEditor(this));
    QAction *a = scriptEditor->getToolBar()->addAction(style()->
            standardIcon(QStyle::SP_DialogSaveButton),
                         "Save", scriptEditor->editor(), SLOT(save()));
    a->setShortcut(tr("Ctrl+S"));
    
    QWidget *w = new QWidget;
    QHBoxLayout *hbox = new QHBoxLayout(w);
    split->addWidget(w);
    
    hbox->addWidget(createLocalLuaWidget());
    hbox->addWidget(createServerLuaWidget());
    
    split->setStretchFactor(0, 66);
    split->setStretchFactor(1, 33);
    return split;
}

QWidget *CQtClient::createNavTab()
{
    QSplitter *split = new QSplitter(Qt::Vertical);

    QGroupBox *group = new QGroupBox("Navigation sim map");
    split->addWidget(group);

    QVBoxLayout *vbox = new QVBoxLayout(group);

    vbox->addWidget(simNavMap = new CNavMap);
    simNavMap->setGrid(QSize(10, 10));
    simNavMap->setRobot(QPoint(0, 0));

    split->addWidget(group = new QGroupBox("Control"));
    QGridLayout *grid = new QGridLayout(group);

    grid->addWidget(startSimNavButton = new QPushButton("Start"), 0, 0);
    connect(startSimNavButton, SIGNAL(clicked()), this, SLOT(startSimNav()));

    grid->addWidget(clearSimNavButton = new QPushButton("Clear map"), 1, 0);
    connect(clearSimNavButton, SIGNAL(clicked()), simNavMap, SLOT(clearCells()));

    grid->addWidget(addSimNavObstacleButton = new QPushButton("Add obstacle"), 2, 0);
    addSimNavObstacleButton->setCheckable(true);
    connect(addSimNavObstacleButton, SIGNAL(toggled(bool)), simNavMap, SLOT(toggleObstacleAdd(bool)));

    QLabel *label = new QLabel("Move time");
    label->setAlignment(Qt::AlignRight);
    grid->addWidget(label, 0, 1);
    grid->addWidget(simMoveTimeSpinBox = new QSpinBox, 0, 2);
    simMoveTimeSpinBox->setRange(0, 10000);
    simMoveTimeSpinBox->setValue(500);

    return split;
}

QWidget *CQtClient::createConsoleTab()
{
    QWidget *ret = new QWidget;
    
    QVBoxLayout *vbox = new QVBoxLayout(ret);
    
    vbox->addWidget(consoleOut = new QPlainTextEdit);
    consoleOut->setReadOnly(true);
    consoleOut->setCenterOnScroll(true);
    
    QHBoxLayout *hbox = new QHBoxLayout;
    vbox->addLayout(hbox);
    
    hbox->addWidget(consoleIn = new QLineEdit);
    
    QPushButton *button = new QPushButton("Send");
    connect(button, SIGNAL(clicked()), this, SLOT(sendConsolePressed()));
    connect(consoleIn, SIGNAL(returnPressed()), button, SLOT(click()));
    hbox->addWidget(button);

    button = new QPushButton("Clear console");
    connect(button, SIGNAL(clicked()), consoleOut, SLOT(clear()));
    hbox->addWidget(button);

    return ret;
}

QWidget *CQtClient::createLogTab()
{
    QWidget *ret = new QWidget;
    
    QVBoxLayout *vbox = new QVBoxLayout(ret);
    
    vbox->addWidget(logWidget = new QPlainTextEdit);
    logWidget->setReadOnly(true);
    logWidget->setCenterOnScroll(true);
    
    QPushButton *button = new QPushButton("Clear log");
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(button, SIGNAL(clicked()), logWidget, SLOT(clear()));
    vbox->addWidget(button, 0, Qt::AlignCenter);
    
    return ret;
}

QWidget *CQtClient::createOverviewWidget(void)
{
    QWidget *ret = new QWidget;
    
    QGridLayout *grid = new QGridLayout(ret);
    
    // Motor group box
    // UNDONE: destination speed, distance
    QGroupBox *group = new QGroupBox("Motor");
    grid->addWidget(group, 0, 0);
    QFormLayout *form = new QFormLayout(group);
    
    QWidget *w = new QWidget;
    QHBoxLayout *hbox = new QHBoxLayout(w);
    hbox->addWidget(motorSpeedLCD[0] = new QLCDNumber);
    hbox->addWidget(motorSpeedLCD[1] = new QLCDNumber);
    form->addRow("Speed", w);
    
    form->addRow("Distance", w = new QWidget);
    hbox = new QHBoxLayout(w);
    hbox->addWidget(motorDistanceLCD[0] = new QLCDNumber);
    hbox->addWidget(motorDistanceLCD[1] = new QLCDNumber);

    form->addRow("Current", w = new QWidget);
    hbox = new QHBoxLayout(w);
    hbox->addWidget(motorCurrentLCD[0] = new QLCDNumber);
    hbox->addWidget(motorCurrentLCD[1] = new QLCDNumber);

    form->addRow("Direction", w = new QWidget);
    hbox = new QHBoxLayout(w);
    hbox->addWidget(motorDirection[0] = createDataLabel("<none>"));
    hbox->addWidget(motorDirection[1] = createDataLabel("<none>"));

    
    // Light sensors group box
    group = new QGroupBox("Optics");
    grid->addWidget(group, 0, 1);
    form = new QFormLayout(group);

    form->addRow("Light sensors", w = new QWidget);
    hbox = new QHBoxLayout(w);
    hbox->addWidget(lightSensorsLCD[0] = new QLCDNumber);
    hbox->addWidget(lightSensorsLCD[1] = new QLCDNumber);
    
    form->addRow("Sharp IR", sharpIRSensor = new QLCDNumber);
    
    // RC5 group box
    group = new QGroupBox("RC5 remote control");
    grid->addWidget(group, 0, 2);
    form = new QFormLayout(group);

    form->addRow("Device", RC5DeviceLabel = createDataLabel("<none>"));
    form->addRow("Toggle bit", RC5ToggleBitBox = new QCheckBox);
    RC5ToggleBitBox->setEnabled(false);
    form->addRow("Key", RC5KeyLabel = createDataLabel("<none>"));

    
    // ACS group box
    group = new QGroupBox("ACS");
    grid->addWidget(group, 1, 0);
    form = new QFormLayout(group);

    form->addRow("ACS", w = new QWidget);
    hbox = new QHBoxLayout(w);
    hbox->addWidget(ACSCollisionBox[0] = new QCheckBox);
    hbox->addWidget(ACSCollisionBox[1] = new QCheckBox);
    ACSCollisionBox[0]->setEnabled(false);
    ACSCollisionBox[1]->setEnabled(false);
    
    form->addRow("Power state", ACSPowerSlider = new QSlider(Qt::Horizontal));
    ACSPowerSlider->setRange(0, 3);
    ACSPowerSlider->setEnabled(false);
    ACSPowerSlider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    
    
    // LEDs group box
    group = new QGroupBox("LEDs");
    grid->addWidget(group, 1, 1);
    form = new QFormLayout(group);
    
    form->addRow("Main LEDs", w = new QWidget);
    QGridLayout *ledgrid = new QGridLayout(w);
    
    for (int col=0; col<2; ++col)
    {
        for (int row=0; row<3; ++row)
        {
            int ind = row + (col * 3);
            ledgrid->addWidget(mainLEDsBox[ind] = new QCheckBox, row, col);
            mainLEDsBox[ind]->setEnabled(false);
        }
    }
    
    form->addRow("M32 LEDs", w = new QWidget);
    hbox = new QHBoxLayout(w);
    for (int i=0; i<4; ++i)
    {
        hbox->addWidget(m32LEDsBox[i] = new QCheckBox);
        m32LEDsBox[i]->setEnabled(false);
    }

    
    // Others group box
    group = new QGroupBox("Other");
    grid->addWidget(group, 1, 2);
    form = new QFormLayout(group);

    form->addRow("Battery", batteryLCD = new QLCDNumber);
    
    form->addRow("Bumpers", w = new QWidget);
    hbox = new QHBoxLayout(w);
    hbox->addWidget(bumperBox[0] = new QCheckBox);
    hbox->addWidget(bumperBox[1] = new QCheckBox);
    bumperBox[0]->setEnabled(false);
    bumperBox[1]->setEnabled(false);
    
    form->addRow("Pressed key", m32KeyLabel = createDataLabel("<none>"));

    return ret;
}

QWidget *CQtClient::createSpeedWidget(void)
{
    QWidget *ret = new QWidget;
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);

    motorSpeedPlot = new CSensorPlot("Motor speed");
    motorSpeedPlot->addSensor("Left (actual)", Qt::red);
    motorSpeedPlot->addSensor("Right (actual)", Qt::blue);
    motorSpeedPlot->addSensor("Left (destination)", Qt::blue);
    motorSpeedPlot->addSensor("Right (destination)", Qt::darkBlue);

    hbox->addWidget(motorSpeedPlot);
    
    return ret;
}

QWidget *CQtClient::createDistWidget(void)
{
    QWidget *ret = new QWidget;
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);

    motorDistancePlot = new CSensorPlot("Motor distance");
    motorDistancePlot->addSensor("Left", Qt::red);
    motorDistancePlot->addSensor("Right", Qt::blue);

    hbox->addWidget(motorDistancePlot);

    return ret;
}

QWidget *CQtClient::createCurrentWidget(void)
{
    QWidget *ret = new QWidget;
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);

    motorCurrentPlot = new CSensorPlot("Motor current");
    motorCurrentPlot->addSensor("Left", Qt::red);
    motorCurrentPlot->addSensor("Right", Qt::blue);

    hbox->addWidget(motorCurrentPlot);

    return ret;
}

QWidget *CQtClient::createLightWidget(void)
{
    QWidget *ret = new QWidget;
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);

    lightSensorsPlot = new CSensorPlot("Light sensors");
    lightSensorsPlot->addSensor("Left", Qt::red);
    lightSensorsPlot->addSensor("Right", Qt::blue);

    hbox->addWidget(lightSensorsPlot);

    return ret;
}

QWidget *CQtClient::createACSWidget(void)
{
    QWidget *ret = new QWidget;
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);

    ACSPlot = new CSensorPlot("Anti Collision Sensors");
    ACSPlot->addSensor("Left", Qt::red, QwtPlotCurve::Steps);
    ACSPlot->addSensor("Right", Qt::blue, QwtPlotCurve::Steps);

    hbox->addWidget(ACSPlot);

    return ret;
}

QWidget *CQtClient::createBatteryWidget(void)
{
    QWidget *ret = new QWidget;
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);

    batteryPlot = new CSensorPlot("Battery voltage");
    batteryPlot->addSensor("Battery", Qt::red);

    hbox->addWidget(batteryPlot);
    
    return ret;
}

QWidget *CQtClient::createMicWidget(void)
{
    QWidget *ret = new QWidget;
    
    QVBoxLayout *vbox = new QVBoxLayout(ret);

    micPlot = new CSensorPlot("Microphone");
    micPlot->addSensor("Microphone", Qt::red);

    vbox->addWidget(micPlot);
    
    QWidget *w = new QWidget;
    w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    vbox->addWidget(w, 0, Qt::AlignCenter);
    
    QHBoxLayout *hbox = new QHBoxLayout(w);
    
    hbox->addWidget(micUpdateTimeSpinBox = new QSpinBox);
    micUpdateTimeSpinBox->setSuffix(" ms");
    micUpdateTimeSpinBox->setRange(0, 100000);
    micUpdateTimeSpinBox->setValue(500);
    micUpdateTimeSpinBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(micUpdateTimeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setMicUpdateTime(int)));
    
    hbox->addWidget(micUpdateToggleButton = new QPushButton("Record..."));
    micUpdateToggleButton->setCheckable(true);
    micUpdateToggleButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(micUpdateToggleButton, SIGNAL(toggled(bool)), this, SLOT(micPlotToggled(bool)));
    connectionDependentWidgets << micUpdateToggleButton;
    
    return ret;
}

QWidget *CQtClient::createSharpIRWidget(void)
{
    QWidget *ret = new QWidget;
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);

    sharpIRPlot = new CSensorPlot("Measered sharp IR distance");
    sharpIRPlot->addSensor("Sharp IR", Qt::red);

    hbox->addWidget(sharpIRPlot);
    
    return ret;
}

QWidget *CQtClient::createConnectionWidget()
{
    QWidget *ret = new QWidget;
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);
    
    hbox->addWidget(serverEdit = new QLineEdit);
    serverEdit->setText("localhost");
    
    hbox->addWidget(connectButton = new QPushButton("Connect"));
    connect(connectButton, SIGNAL(clicked()), this, SLOT(toggleServerConnection()));
    
    return ret;
}

QWidget *CQtClient::createDriveWidget()
{
    QWidget *ret = new QWidget;
    connectionDependentWidgets << ret;
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);
    
    // Arrow box
    QWidget *w = new QWidget;
    w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    hbox->addWidget(w);
    
    QGridLayout *grid = new QGridLayout(w);
    
    driveMapper = new QSignalMapper(this);
    connect(driveMapper, SIGNAL(mapped(int)), this, SLOT(driveButtonPressed(int)));
    
    QPushButton *button = createDriveButton(style()->standardIcon(QStyle::SP_ArrowUp), Qt::Key_Up);
    connect(button, SIGNAL(clicked()), driveMapper, SLOT(map()));
    driveMapper->setMapping(button, FWD);
    grid->addWidget(button, 0, 1);
    
    button = createDriveButton(style()->standardIcon(QStyle::SP_ArrowLeft), Qt::Key_Left);
    connect(button, SIGNAL(clicked()), driveMapper, SLOT(map()));
    driveMapper->setMapping(button, LEFT);
    grid->addWidget(button, 1, 0);
    
    button = createDriveButton(style()->standardIcon(QStyle::SP_ArrowDown), Qt::Key_Down);
    connect(button, SIGNAL(clicked()), driveMapper, SLOT(map()));
    driveMapper->setMapping(button, BWD);
    grid->addWidget(button, 1, 1);

    button = createDriveButton(style()->standardIcon(QStyle::SP_ArrowRight), Qt::Key_Right);
    connect(button, SIGNAL(clicked()), driveMapper, SLOT(map()));
    driveMapper->setMapping(button, RIGHT);
    grid->addWidget(button, 1, 2);
    
    // Others
    button = new QPushButton("Full stop");
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(button, SIGNAL(clicked()), this, SLOT(stopDriveButtonPressed()));
    hbox->addWidget(button);
    
    hbox->addWidget(createSlider("<qt><strong>Left</strong>", driveSpeedSlider[0], -255, 255));
    hbox->addWidget(createSlider("<qt><strong>Right</strong>", driveSpeedSlider[1], -255, 255));
    
    return ret;
}

QWidget *CQtClient::createScannerWidget()
{
    QWidget *ret = new QWidget;
    connectionDependentWidgets << ret;
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);
    
    QWidget *w = new QWidget;
    w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    hbox->addWidget(w, 0, Qt::AlignVCenter);
    QFormLayout *form = new QFormLayout(w);
    
    form->addRow("Speed", ACSScanSpeedSpinBox = new QSpinBox);
    ACSScanSpeedSpinBox->setRange(0, 160);
    ACSScanSpeedSpinBox->setValue(60);

    form->addRow("Scan power", ACSScanPowerComboBox = new QComboBox);
    ACSScanPowerComboBox->addItems(QStringList() << "Low" << "Medium" << "High" <<
            "Alternating");
    ACSScanPowerComboBox->setCurrentIndex(1);
    
    form->addRow(ACSScanButton = new QPushButton("Scan"));
    connect(ACSScanButton, SIGNAL(clicked()), this, SLOT(ACSScanButtonPressed()));
    
    
    hbox->addWidget(ACSScannerWidget = new CScannerWidget(3), 0);

    return ret;
}

QWidget *CQtClient::createIRTurretWidget()
{
    QWidget *ret = new QWidget;
    connectionDependentWidgets << ret;
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);
    
    QGroupBox *group = new QGroupBox("Servo control");
    hbox->addWidget(group);
    
    QHBoxLayout *subhbox = new QHBoxLayout(group);
    
    subhbox->addWidget(servoDial = new QDial);
    servoDial->setRange(0, 180);
    servoDial->setNotchesVisible(true);
    
    subhbox->addWidget(servoButton = new QPushButton("Apply"));
    connect(servoButton, SIGNAL(clicked()), this, SLOT(servoButtonPressed()));
    
    
    hbox->addWidget(group = new QGroupBox("Scanner"), 1);
    
    subhbox = new QHBoxLayout(group);
    
    QFormLayout *form = new QFormLayout;
    subhbox->addLayout(form);
    
    QWidget *w = new QWidget;
    form->addRow("Range", w);
    QHBoxLayout *rhbox = new QHBoxLayout(w);
    
    rhbox->addWidget(turretScanRangeSpinBox[0] = new QSpinBox);
    turretScanRangeSpinBox[0]->setRange(0, 179);
    turretScanRangeSpinBox[0]->setValue(0);
    turretScanRangeSpinBox[0]->setSuffix(QChar(0x00B0)); // Degree

    rhbox->addWidget(turretScanRangeSpinBox[1] = new QSpinBox);
    turretScanRangeSpinBox[1]->setRange(1, 180);
    turretScanRangeSpinBox[1]->setValue(180);
    turretScanRangeSpinBox[1]->setSuffix(QChar(0x00B0)); // Degree
    
    form->addRow("Resolution", turretScanResolutionSpinBox = new QSpinBox);
    turretScanResolutionSpinBox->setRange(1, 90);
    turretScanResolutionSpinBox->setValue(30);
    turretScanResolutionSpinBox->setSuffix(QChar(0x00B0)); // Degree
    
    form->addRow("Measure time", turretScanTimeSpinBox = new QSpinBox);
    turretScanTimeSpinBox->setRange(50, 30000);
    turretScanTimeSpinBox->setValue(250);
    turretScanTimeSpinBox->setSuffix(" ms");
    
    form->addRow("Scan delay", turretScanDelaySpinBox = new QSpinBox);
    turretScanDelaySpinBox->setRange(0, 30000);
    turretScanDelaySpinBox->setValue(50);
    turretScanDelaySpinBox->setSuffix(" ms");

    form->addWidget(turretScanButton = new QPushButton("Scan"));
    connect(turretScanButton, SIGNAL(clicked()), this, SLOT(turretScanButtonPressed()));
    
    subhbox->addWidget(turrentScannerWidget = new CScannerWidget(150, -90, 90), 0);
    turrentScannerWidget->addPoint(0, 0);
    turrentScannerWidget->addPoint(60, 0);

    return ret;
}

QWidget *CQtClient::createLocalLuaWidget()
{
    QGroupBox *ret = new QGroupBox("Local");
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);
    
    hbox->addWidget(localScriptListWidget = new QListWidget);
    connect(localScriptListWidget, SIGNAL(itemActivated(QListWidgetItem *)), this,
            SLOT(localScriptChanged(QListWidgetItem *)));

    QSettings settings;
    QStringList scripts = settings.value("scripts").toStringList();
    for (QStringList::iterator it=scripts.begin(); it!=scripts.end(); ++it)
    {
        QListWidgetItem *item = new QListWidgetItem(QFileInfo(*it).fileName(),
                localScriptListWidget);
        item->setData(Qt::UserRole, *it);
    }
    
    QVBoxLayout *vbox = new QVBoxLayout;
    hbox->addLayout(vbox);
    
    QPushButton *button = new QPushButton("New");
    connect(button, SIGNAL(clicked()), this, SLOT(newLocalScriptPressed()));
    vbox->addWidget(button);
    
    vbox->addWidget(button = new QPushButton("Add"));
    connect(button, SIGNAL(clicked()), this, SLOT(addLocalScriptPressed()));

    vbox->addWidget(button = new QPushButton("Remove"));
    connect(button, SIGNAL(clicked()), this, SLOT(removeLocalScriptPressed()));
    
    vbox->addWidget(button = new QPushButton("Upload"));
    connect(button, SIGNAL(clicked()), this, SLOT(uploadLocalScriptPressed()));
    connectionDependentWidgets << button;
    
    vbox->addWidget(button = new QPushButton("Run"));
    connect(button, SIGNAL(clicked()), this, SLOT(runLocalScriptPressed()));
    connectionDependentWidgets << button;
    
    vbox->addWidget(button = new QPushButton("Upload && Run"));
    connect(button, SIGNAL(clicked()), this, SLOT(uploadRunLocalScriptPressed()));
    connectionDependentWidgets << button;
    
    return ret;
}

QWidget *CQtClient::createServerLuaWidget()
{
    QGroupBox *ret = new QGroupBox("Fox");
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);
    
    hbox->addWidget(serverScriptListWidget = new QListWidget);
    
    QVBoxLayout *vbox = new QVBoxLayout;
    hbox->addLayout(vbox);
    
    QPushButton *button = new QPushButton("Run");
    vbox->addWidget(button);
    connect(button, SIGNAL(clicked()), this, SLOT(runServerScriptPressed()));
    connectionDependentWidgets << button;
    
    vbox->addWidget(button = new QPushButton("Remove"));
    connect(button, SIGNAL(clicked()), this, SLOT(removeServerScriptPressed()));
    connectionDependentWidgets << button;
    
    vbox->addWidget(downloadButton = new QPushButton("Download"));
    connect(downloadButton, SIGNAL(clicked()), this, SLOT(downloadServerScriptPressed()));
    connectionDependentWidgets << downloadButton;
    
    return ret;
}

void CQtClient::updateACSScan(const SStateSensors &oldstate, const SStateSensors &newstate)
{
    appendLogOutput("updateACSScan()\n");
    
    // UNDONE: Which sensor?
    if (newstate.ACSLeft || newstate.ACSRight)
        ACSScannerWidget->addPoint(motorDistance[0] * 360 / destMotorDistance[0],
                                ACSPowerState);
    else
        ACSScannerWidget->endPoint();
    
    if (!firstStateUpdate && !oldstate.movementComplete && newstate.movementComplete)
    {
        stopACSScan();
        return;
    }

    if (alternatingACSScan && (ACSPowerState != ACS_POWER_OFF))
    {
        if (!remainingACSScanCycles && (newstate.ACSState < 2))
        {
            appendLogOutput("ACS Cycle reached\n");
            
            // UNDONE: Only change to more power when no hit
            EACSPowerState newpower = ACSPowerState;
            if (newpower == ACS_POWER_LOW)
                newpower = ACS_POWER_MED;
            else if (newpower == ACS_POWER_MED)
                newpower = ACS_POWER_HIGH;
            else // if (newacs == ACS_POWER_HIGH)
                newpower = ACS_POWER_LOW;
            
            remainingACSScanCycles = 2; // Wait two more cycles before next switch
            
            executeCommand(QString("set acs %1").arg(ACSPowerToString(newpower)));
        }
        
        if (remainingACSScanCycles && (oldstate.ACSState != ACS_STATE_WAIT_RIGHT) &&
            (newstate.ACSState == ACS_STATE_WAIT_RIGHT))
            --remainingACSScanCycles; // Reached another state cycle
    }
}

void CQtClient::stopACSScan()
{
    isACSScanning = false;
    ACSScanButton->setEnabled(true);
}

void CQtClient::setServo(int pos)
{
    executeCommand(QString("set servo %1").arg(pos));
    servoDial->setValue(pos);
}

bool CQtClient::checkScriptSave()
{
    if (scriptEditor->editor()->isContentModified())
    {
        QMessageBox::StandardButton ret = QMessageBox::question(this,
                "Save modified script?", QString("Changes has been made to %1.\nSave?").
                arg(scriptEditor->editor()->fileName()),
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                QMessageBox::Yes);
        
        if (ret == QMessageBox::Cancel)
            return false;
        else if (ret == QMessageBox::Yes)
            scriptEditor->editor()->save();
    }
    
    return true;
}

void CQtClient::stopSimNav()
{
    simNavTimer->stop();
    startSimNavButton->setText("Start");
    clearSimNavButton->setEnabled(true);
    addSimNavObstacleButton->setEnabled(true);
}

void CQtClient::updateConnection(bool connected)
{
    if (connected)
    {
        firstStateUpdate = true;
        connectButton->setText("Disconnect");
    }
    else
        connectButton->setText("Connect");
    
    for (QList<QWidget *>::iterator it=connectionDependentWidgets.begin();
         it!=connectionDependentWidgets.end(); ++it)
        (*it)->setEnabled(connected);
}

void CQtClient::tcpError(const QString &error)
{
    QMessageBox::critical(this, "Socket error", error);
}

void CQtClient::tcpRobotStateUpdate(const SStateSensors &oldstate,
                                    const SStateSensors &newstate)
{
    bumperBox[0]->setChecked(newstate.bumperLeft);
    bumperBox[1]->setChecked(newstate.bumperRight);
    
    ACSCollisionBox[0]->setChecked(newstate.ACSLeft);
    ACSCollisionBox[1]->setChecked(newstate.ACSRight);
    
    ACSPlot->addData("Left", newstate.ACSLeft * ACSPowerSlider->value());
    ACSPlot->addData("Right", newstate.ACSRight * ACSPowerSlider->value());
    
    if (isACSScanning)
        updateACSScan(oldstate, newstate);
    
    firstStateUpdate = false;
}

void CQtClient::tcpHandleRobotData(ETcpMessage msg, int data)
{
    switch (msg)
    {
        case TCP_BASE_LEDS:
        {
            for (int i=0; i<6; ++i)
                mainLEDsBox[i]->setChecked(data & (1<<i));
            break;
        }
        case TCP_M32_LEDS:
        {
            for (int i=0; i<4; ++i)
                m32LEDsBox[i]->setChecked(data & (1<<i));
            break;
        }
        case TCP_MOTOR_DIRECTIONS:
        {
            SMotorDirections dir;
            dir.byte = data;
            motorDirection[0]->setText(directionString(dir.left));
            motorDirection[1]->setText(directionString(dir.right));
            break;
        }
        case TCP_MOTOR_DESTDIST_LEFT: destMotorDistance[0] = data; break;
        case TCP_MOTOR_DESTDIST_RIGHT: destMotorDistance[1] = data; break;
        case TCP_ACS_POWER:
            ACSPowerSlider->setValue(data);
            ACSPowerState = static_cast<EACSPowerState>(data);
            break;
        case TCP_LASTRC5:
        {
            RC5data_t rc5;
            rc5.data = data;
            RC5DeviceLabel->setText(QString::number(rc5.device));
            RC5KeyLabel->setText(QString::number(rc5.key_code));
            RC5ToggleBitBox->setChecked(rc5.toggle_bit);
            break;
        }
        
        // Averaged data?
        case TCP_LIGHT_LEFT:
        case TCP_LIGHT_RIGHT:
        case TCP_MOTOR_SPEED_LEFT:
        case TCP_MOTOR_SPEED_RIGHT:
        case TCP_MOTOR_CURRENT_LEFT:
        case TCP_MOTOR_CURRENT_RIGHT:
        case TCP_BATTERY:
        case TCP_MIC:
            averagedSensorDataMap[msg].total += data;
            averagedSensorDataMap[msg].count++;
            break;

        case TCP_MOTOR_DESTSPEED_LEFT:
        case TCP_MOTOR_DESTSPEED_RIGHT:
            delayedSensorDataMap[msg] = data;
            break;

        case TCP_MOTOR_DIST_LEFT:
            motorDistance[0] = data;
            delayedSensorDataMap[msg] = data;
            break;
            
        case TCP_MOTOR_DIST_RIGHT:
            motorDistance[1] = data;
            delayedSensorDataMap[msg] = data;
            break;
            
        case TCP_SHARPIR:
            if (isTurretScanning)
                turretScanData << data;
            
            averagedSensorDataMap[msg].total += data;
            averagedSensorDataMap[msg].count++;
            break;

        default: break;
    }
}

void CQtClient::tcpLuaScripts(const QStringList &list)
{
    serverScriptListWidget->clear();
    serverScriptListWidget->addItems(list);
}

void CQtClient::tcpRequestedScript(const QByteArray &text)
{
    QString fn = QFileDialog::getSaveFileName(this, "Save script", downloadScript,
                                              tr("Lua scripts (*.lua)"));
        
    if (!fn.isEmpty())
    {
        QFile file(fn);
        if (!file.open(QFile::WriteOnly | QFile::Truncate | QFile::Text))
            QMessageBox::critical(this, "File error",
                                  QString("Failed to create file"));
        else
        {
            file.write(text);
                
            QSettings settings;
            QStringList scripts = settings.value("scripts").toStringList();
            scripts << fn;
            settings.setValue("scripts", scripts);

            QListWidgetItem *item = new QListWidgetItem(QFileInfo(file).fileName(),
                    localScriptListWidget);
            item->setData(Qt::UserRole, fn);
            localScriptListWidget->setCurrentItem(item);
        }
    }
        
    downloadScript.clear();
    downloadButton->setEnabled(true);
}

void CQtClient::updateDriveSpeed(int left, int right)
{
    driveSpeedSlider[0]->setValue(left);
    driveSpeedSlider[1]->setValue(right);
}

void CQtClient::updateSensors()
{
    for (QMap<ETcpMessage, SSensorData>::iterator it=averagedSensorDataMap.begin();
         it!=averagedSensorDataMap.end(); ++it)
    {
        if (it.value().count == 0)
            continue;
        
        int data = it.value().total / it.value().count;
        
        if (it.key() == TCP_LIGHT_LEFT)
        {
            lightSensorsLCD[0]->display(data);
            lightSensorsPlot->addData("Left", data);
        }
        else if (it.key() == TCP_LIGHT_RIGHT)
        {
            lightSensorsLCD[1]->display(data);
            lightSensorsPlot->addData("Right", data);
        }
        else if (it.key() == TCP_MOTOR_SPEED_LEFT)
        {
            motorSpeedLCD[0]->display(data);
            motorSpeedPlot->addData("Left (actual)", data);
        }
        else if (it.key() == TCP_MOTOR_SPEED_RIGHT)
        {
            motorSpeedLCD[1]->display(data);
            motorSpeedPlot->addData("Right (actual)", data);
        }
        else if (it.key() == TCP_MOTOR_CURRENT_LEFT)
        {
            motorCurrentLCD[0]->display(data);
            motorCurrentPlot->addData("Left", data);
        }
        else if (it.key() == TCP_MOTOR_CURRENT_RIGHT)
        {
            motorCurrentLCD[1]->display(data);
            motorCurrentPlot->addData("Right", data);
        }
        else if (it.key() == TCP_BATTERY)
        {
            batteryLCD->display(data);
            batteryPlot->addData("Battery", data);
        }
        else if (it.key() == TCP_MIC)
            micPlot->addData("Microphone", data);
        else if (it.key() == TCP_SHARPIR)
        {
            sharpIRSensor->display(data);
            sharpIRPlot->addData("Sharp IR", data);
        }
        
        it.value().total = it.value().count = 0;
    }
    
    motorSpeedPlot->addData("Left (destination)",
                            delayedSensorDataMap[TCP_MOTOR_DESTSPEED_LEFT]);
    
    motorSpeedPlot->addData("Right (destination)",
                            delayedSensorDataMap[TCP_MOTOR_DESTSPEED_RIGHT]);
    
    motorDistanceLCD[0]->display(delayedSensorDataMap[TCP_MOTOR_DIST_LEFT]);
    motorDistancePlot->addData("Left", delayedSensorDataMap[TCP_MOTOR_DIST_LEFT]);
    
    motorDistanceLCD[1]->display(delayedSensorDataMap[TCP_MOTOR_DIST_RIGHT]);
    motorDistancePlot->addData("Right", delayedSensorDataMap[TCP_MOTOR_DIST_RIGHT]);
}

void CQtClient::toggleServerConnection()
{
    if (connected())
        disconnectFromServer();
    else
        connectToHost(serverEdit->text());
}

void CQtClient::setMicUpdateTime(int value)
{
    if (micUpdateToggleButton->isChecked())
        executeCommand(QString("set slavemic %1").arg(value));
}

void CQtClient::micPlotToggled(bool checked)
{
    if (checked)
        executeCommand(QString("set slavemic %1").arg(micUpdateTimeSpinBox->value()));
    else
        executeCommand("set slavemic 0");
}

void CQtClient::ACSScanButtonPressed()
{
    isACSScanning = true;
    ACSScannerWidget->clear();
    ACSScanButton->setEnabled(false);
    
    if (ACSScanPowerComboBox->currentText() == "Alternating")
    {
        appendLogOutput("Alternating scan started.\n");
        executeCommand("set acs low");
        remainingACSScanCycles = 2;
        alternatingACSScan = true;
    }
    else
    {
        alternatingACSScan = false;
        
        if (ACSScanPowerComboBox->currentText() == "Low")
            executeCommand("set acs low");
        else if (ACSScanPowerComboBox->currentText() == "Medium")
            executeCommand("set acs med");
        else if (ACSScanPowerComboBox->currentText() == "High")
            executeCommand("set acs high");
    }
    
    executeCommand(QString("rotate 360 %1").arg(ACSScanSpeedSpinBox->value()));
}

void CQtClient::servoButtonPressed()
{
    executeCommand(QString("set servo %1").arg(servoDial->value()));
}

void CQtClient::turretScanButtonPressed()
{
    servoButton->setEnabled(false);
    turretScanButton->setEnabled(false);
    
    turrentScannerWidget->clear();
    
    currentScanPosition = turretScanRangeSpinBox[0]->value();
    turretScanEndRange = turretScanRangeSpinBox[1]->value();
    turretScanResolution = turretScanResolutionSpinBox->value();
    turretScanTime = turretScanTimeSpinBox->value();
    turretScanDelay = turretScanDelaySpinBox->value();
    turretScanData.clear();
    
    turretScanTimer->start(turretScanDelay);
    
    setServo(currentScanPosition);
    
    appendLogOutput("Started turret scan.\n");
}

void CQtClient::turretScanTimeout()
{
    if (!isTurretScanning) // Init scan?
    {
        isTurretScanning = true; // Enable data collection
        turretScanTimer->setInterval(turretScanTime);
        return;
    }

    if (turretScanData.isEmpty())
        appendLogOutput("WARNING: No sharp IR measurements!\n");
    else
    {
        QStringList set; // UNDONE: Remove
        int dist = 0;
        for (QList<int>::iterator it=turretScanData.begin(); it!=turretScanData.end(); ++it)
        {
            dist += *it;
            set << QString::number(*it);
        }
        
        dist /= turretScanData.size(); // Mean
        const float stdev = standardDeviation(turretScanData);
        const float RSD = stdev / dist * 100.0;
        
        if ((dist >= 20) && (dist <= 150)) // Only take valid ranges (UNDONE: Move elsewhere?)
        {
            turrentScannerWidget->addPoint(currentScanPosition - 90, dist);
            appendLogOutput(QString("Turret scan: %1 cm (n=%2, s=%3, RSD=%4%, set: [%5])\n").
                    arg(dist).arg(turretScanData.size()).arg(stdev).arg(RSD).arg(set.join(", ")));
        }
        else
            turrentScannerWidget->endPoint();
        
        turretScanData.clear();
    }
    
    currentScanPosition += turretScanResolution;

    // Finished?
    if (currentScanPosition > turretScanEndRange)
    {
        isTurretScanning = false;
        servoButton->setEnabled(true);
        turretScanButton->setEnabled(true);
        turretScanTimer->stop();
        appendLogOutput("Finished turret scan.\n");
        return;
    }
    
    setServo(currentScanPosition);
    
    // Delay measurements
    isTurretScanning = false;
    turretScanTimer->setInterval(turretScanDelay);
}

void CQtClient::localScriptChanged(QListWidgetItem *item)
{
    const QString fn = item->data(Qt::UserRole).toString();
    
    if ((fn != scriptEditor->editor()->fileName()) && !checkScriptSave())
        localScriptListWidget->setCurrentItem(previousScriptItem);
    else
    {
        if (!QFileInfo(fn).exists())
        {
            scriptEditor->editor()->setFileName("");
            scriptEditor->setText(QString("Could not open file %1.").arg(fn));
        }
        else
            scriptEditor->load(fn);

        previousScriptItem = item;
    }
}

void CQtClient::newLocalScriptPressed()
{
    QString file = QFileDialog::getSaveFileName(this, QString(), QString(),
            tr("Lua scripts (*.lua)"));
    
    if (!file.isNull())
    {
        QFile f(file);
        if (!f.open(QFile::WriteOnly | QFile::Truncate | QFile::Text))
            QMessageBox::critical(this, "File error", QString("Failed to create file"));
        else
        {
            QSettings settings;
            QStringList scripts = settings.value("scripts").toStringList();
            scripts << file;
            settings.setValue("scripts", scripts);

            QListWidgetItem *item = new QListWidgetItem(QFileInfo(file).fileName(),
                    localScriptListWidget);
            item->setData(Qt::UserRole, file);
            localScriptListWidget->setCurrentItem(item);
        }
    }
}

void CQtClient::addLocalScriptPressed()
{
    QString file = QFileDialog::getOpenFileName(this, QString(), QString(),
            tr("Lua scripts (*.lua)"));
    
    if (!file.isNull())
    {
        QSettings settings;
        QStringList scripts = settings.value("scripts").toStringList();
        scripts << file;
        settings.setValue("scripts", scripts);

        QListWidgetItem *item = new QListWidgetItem(QFileInfo(file).fileName(),
                localScriptListWidget);
        item->setData(Qt::UserRole, file);
        localScriptListWidget->setCurrentItem(item);
    }
}

void CQtClient::removeLocalScriptPressed()
{
    if (localScriptListWidget->currentRow() != -1)
    {
        QMessageBox::StandardButton ret = QMessageBox::question(this,
                "Removing file", "Remove from disk aswell?",
                            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                            QMessageBox::Yes);
        if (ret != QMessageBox::Cancel)
        {
            if (ret == QMessageBox::Yes)
                QFile::remove(localScriptListWidget->currentItem()->
                        data(Qt::UserRole).toString());
            
            QSettings settings;
            QStringList scripts = settings.value("scripts").toStringList();
            scripts.removeAt(localScriptListWidget->currentRow());
            settings.setValue("scripts", scripts);
        
            delete localScriptListWidget->currentItem();
        }
    }
}

void CQtClient::uploadLocalScriptPressed()
{
    if (localScriptListWidget->currentItem())
    {
        QFile file(localScriptListWidget->currentItem()->data(Qt::UserRole).toString());
        if (!file.open(QFile::ReadOnly | QFile::Text))
            QMessageBox::critical(this, "File error", QString("Failed to open file."));
        else
            uploadLocalScript(localScriptListWidget->currentItem()->text(),
                              file.readAll());
    }
}

void CQtClient::runLocalScriptPressed()
{
    if (localScriptListWidget->currentItem())
    {
        QFile file(localScriptListWidget->currentItem()->data(Qt::UserRole).toString());
        if (!file.open(QFile::ReadOnly | QFile::Text))
            QMessageBox::critical(this, "File error", QString("Failed to open file."));
        else
            runLocalScript(file.readAll());
    }
}

void CQtClient::uploadRunLocalScriptPressed()
{
    if (localScriptListWidget->currentItem())
    {
        QFile file(localScriptListWidget->currentItem()->data(Qt::UserRole).toString());
        if (!file.open(QFile::ReadOnly | QFile::Text))
            QMessageBox::critical(this, "File error", QString("Failed to open file."));
        else
            uploadRunLocalScript(localScriptListWidget->currentItem()->text(),
                                 file.readAll());
    }
}

void CQtClient::runServerScriptPressed()
{
    if (serverScriptListWidget->currentItem())
        runServerScript(serverScriptListWidget->currentItem()->text());
}

void CQtClient::removeServerScriptPressed()
{
    if (serverScriptListWidget->currentItem())
        removeServerScript(serverScriptListWidget->currentItem()->text());
}

void CQtClient::downloadServerScriptPressed()
{
    if (serverScriptListWidget->currentItem())
    {
        downloadScript = serverScriptListWidget->currentItem()->text();
        downloadButton->setEnabled(false);
        downloadServerScript(downloadScript);
    }
}

void CQtClient::startSimNav()
{
    // Already running? --> Abort
    if (simNavTimer->isActive())
        stopSimNav();
    else
    {
        simNavMap->clearConnections();
        simNavMap->setRobot(QPoint(0, 0));
        simNavTimer->start(simMoveTimeSpinBox->value());

        startSimNavButton->setText("Abort");
        clearSimNavButton->setEnabled(false);
        addSimNavObstacleButton->setEnabled(false);
    }
}

void CQtClient::simNavTimeout()
{
    const QPoint currentPos = simNavMap->getRobot();
    const QSize gridSize = simNavMap->getGridSize();

    if (currentPos.x()+1 >= gridSize.width())
    {
        if (currentPos.y()+1 >= gridSize.height())
            stopSimNav();
        else
        {
            simNavMap->setRobot(QPoint(0, currentPos.y()+1));
        }
    }
    else
    {
        QPoint pos = currentPos;
        pos.rx()++;
        simNavMap->setRobot(pos);
    }
}

void CQtClient::sendConsolePressed()
{
    // Append cmd
    appendConsoleOutput(QString("> %1\n").arg(consoleIn->text()));
    executeCommand(consoleIn->text());
    consoleIn->clear();
}

void CQtClient::appendConsoleOutput(const QString &text)
{
    QTextCursor cur = consoleOut->textCursor();
    cur.movePosition(QTextCursor::End);
    consoleOut->setTextCursor(cur);
    consoleOut->insertPlainText(text);
}

void CQtClient::appendLogOutput(const QString &text)
{
    logWidget->appendHtml(QString("<FONT color=#FF0000><strong>[%1]</strong></FONT> %2")
            .arg(QTime::currentTime().toString()).arg(text));
}

void CQtClient::closeEvent(QCloseEvent *e)
{
    if (checkScriptSave())
        e->accept();
    else
        e->ignore();
}
