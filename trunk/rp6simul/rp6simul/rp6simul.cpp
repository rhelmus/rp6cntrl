#include "rp6simul.h"
#include "avrtimer.h"
#include "bumper.h"
#include "irsensor.h"
#include "led.h"
#include "lightsensor.h"
#include "lua.h"
#include "mapsettingsdialog.h"
#include "pathinput.h"
#include "projectsettings.h"
#include "projectwizard.h"
#include "resizablepixmapgraphicsitem.h"
#include "robotgraphicsitem.h"
#include "robotwidget.h"
#include "simulator.h"
#include "utils.h"

#include "qextserialport.h"

#include <QtGui>

namespace {

void activateDockTab(QObject *p, const QString &tab)
{
    // Based on http://www.qtcentre.org/threads/21362-Setting-the-active-tab-with-tabified-docking-windows
    QList<QTabBar *> tabs = p->findChildren<QTabBar *>();
    foreach (QTabBar *t, tabs)
    {
        const int count = t->count();
        for (int i=0; i<count; ++i)
        {
            if (t->tabText(i) == tab)
            {
                t->setCurrentIndex(i);
                return;
            }
        }
    }

    qDebug() << "Warning: couldn't activate dock tab" << tab << "(not found)";
}

QByteArray handSerialDeviceData(QextSerialPort *dev, lua_State *l, int luacb)
{
    QByteArray bytes;
    bytes.resize(dev->bytesAvailable());
    if (dev->read(bytes.data(), bytes.size()) == -1)
        return QByteArray();

    qDebug() << dev->portName() << ":" << bytes;

    NLua::CLuaLocker lualocker(l);
    lua_rawgeti(l, LUA_REGISTRYINDEX, luacb);
    lua_pushstring(l, bytes.constData());
    lua_call(l, 1, 0);

    return bytes;
}

}

CRP6Simulator *CRP6Simulator::instance = 0;

CRP6Simulator::CRP6Simulator(QWidget *parent) :
    QMainWindow(parent), currentMapIsTemplate(false), robotIsBlocked(false),
    robotSerialSendLuaCallback(0), m32SerialSendLuaCallback(0),
    IRCOMMSendLuaCallback(0)
{
    Q_ASSERT(!instance);
    instance = this;

    qRegisterMetaType<EMotor>("EMotor");
    qRegisterMetaType<EMotorDirection>("EMotorDirection");

    resize(850, 600);

    projectWizard = new CProjectWizard(this);

    QSplitter *splitter = new QSplitter(Qt::Vertical, this);
    setCentralWidget(splitter);

    splitter->addWidget(createMainWidget());
    splitter->addWidget(createBottomTabs());
    splitter->setSizes(QList<int>() << 600 << 300);

    createMenus();
    createToolbars();

    updateMainStackedWidget();

    QDockWidget *statdock, *regdock;
    addDockWidget(Qt::LeftDockWidgetArea, statdock = createStatusDock(),
                  Qt::Vertical);
    addDockWidget(Qt::LeftDockWidgetArea, ADCDockWidget = createADCDock(),
                  Qt::Vertical);
    addDockWidget(Qt::LeftDockWidgetArea, regdock = createRegisterDock(),
                  Qt::Vertical);
    tabifyDockWidget(statdock, ADCDockWidget);
    tabifyDockWidget(ADCDockWidget, regdock);
    activateDockTab(this, "Status");

    ADCDockWidget->setEnabled(false);

    // Do this after toolbars and dockedwidgets are created, as the
    // tabViewChanged slot needs some.
    connect(projectTabWidget, SIGNAL(currentChanged(int)),
            SLOT(tabViewChanged(int)));

    pluginUpdateUITimer = new QTimer(this);
    connect(pluginUpdateUITimer, SIGNAL(timeout()), this,
            SLOT(timedUIUpdate()));
    pluginUpdateUITimer->setInterval(250);

    // Faster timer for LEDs
    pluginUpdateLEDsTimer = new QTimer(this);
    connect(pluginUpdateLEDsTimer, SIGNAL(timeout()), this,
            SLOT(timedLEDUpdate()));
    pluginUpdateLEDsTimer->setInterval(50);

    // UNDONE: port config and only open if set
    robotSerialDevice = new QextSerialPort("/dev/pts/3",
                                           QextSerialPort::EventDriven);
    connect(robotSerialDevice, SIGNAL(readyRead()),
            SLOT(handleRobotSerialDeviceData()));
    initSerialPort(robotSerialDevice);
    openSerialPort(robotSerialDevice);

    m32SerialDevice = new QextSerialPort("/dev/pts/4",
                                           QextSerialPort::EventDriven);
    connect(m32SerialDevice, SIGNAL(readyRead()),
            SLOT(handleM32SerialDeviceData()));
    initSerialPort(m32SerialDevice);
    openSerialPort(m32SerialDevice);

    initSimulators();


    // UNDONE
    openProjectFile("/home/rick/test.rp6");
}

CRP6Simulator::~CRP6Simulator()
{
    // Stop plugin manually: ensures safe thread destruction before anything
    // else is destroyed
    stopPlugin();

    // Destruction happens automaticallty by Qt's parent system
    robotSimulator = 0;
    m32Simulator = 0;

    instance = 0;
    NLua::clearLuaLockStates();
}

void CRP6Simulator::initSerialPort(QextSerialPort *port)
{
    port->setParent(this);
    // UNDONE: Config
    port->setBaudRate(BAUD38400);
    port->setFlowControl(FLOW_OFF);
    port->setParity(PAR_NONE);
    port->setDataBits(DATA_8);
    port->setStopBits(STOP_1);
}

void CRP6Simulator::openSerialPort(QextSerialPort *port)
{
    if (port->open(QIODevice::ReadWrite) == true)
    {
        qDebug() << "listening for data on" << port->portName();
        port->setDtr(false);
        port->setRts(false);
    }
    else
        QMessageBox::critical(this, "Serial port error",
                              QString("Failed to open serial port:\n%1").arg(port->errorString()));
}

void CRP6Simulator::initSimulators()
{
    robotSimulator = new CSimulator(this);
    connect(robotSimulator->getAVRClock(), SIGNAL(clockSpeed(unsigned long)),
            SLOT(updateRobotClockDisplay(unsigned long)));
    registerLuaGeneric(robotSimulator->getLuaState());
    registerLuaRobot(robotSimulator->getLuaState());
    robotSimulator->startLua("robot");

    m32Simulator = new CSimulator(this);
    connect(m32Simulator->getAVRClock(), SIGNAL(clockSpeed(unsigned long)),
            SLOT(updateM32ClockDisplay(unsigned long)));
    registerLuaGeneric(m32Simulator->getLuaState());
    registerLuaM32(m32Simulator->getLuaState());
    m32Simulator->startLua("m32");

    connect(robotSimulator, SIGNAL(luaTWIMSGSend(QString,QList<QVariant>)),
            m32Simulator, SLOT(handleLuaTWIMSG(QString,QList<QVariant>)),
            Qt::QueuedConnection);
    connect(m32Simulator, SIGNAL(luaTWIMSGSend(QString,QList<QVariant>)),
            robotSimulator, SLOT(handleLuaTWIMSG(QString,QList<QVariant>)),
            Qt::QueuedConnection);
}

void CRP6Simulator::registerLuaGeneric(lua_State *l)
{
    NLua::registerFunction(l, luaAppendLogOutput, "appendLogOutput");
    NLua::registerFunction(l, luaUpdateRobotStatus, "updateRobotStatus");
}

void CRP6Simulator::registerLuaRobot(lua_State *l)
{
    NLua::registerFunction(l, luaSetCmPerPixel, "setCmPerPixel");
    NLua::registerFunction(l, luaSetRobotLength, "setRobotLength");
    NLua::registerFunction(l, luaSetRobotM32Slot, "setRobotM32Slot");
    NLua::registerFunction(l, luaSetRobotM32Scale, "setRobotM32Scale");
    NLua::registerFunction(l, luaSetRobotSerialSendCallback,
                           "setSerialSendCallback");
    NLua::registerFunction(l, luaAppendRobotSerialOutput, "appendSerialOutput");
    NLua::registerFunction(l, luaLogIRCOMM, "logIRCOMM");
    NLua::registerFunction(l, luaRobotIsBlocked, "robotIsBlocked");
    NLua::registerFunction(l, luaSetMotorPower, "setMotorPower");
    NLua::registerFunction(l, luaSetMotorDriveSpeed, "setMotorDriveSpeed");
    NLua::registerFunction(l, luaSetMotorMoveSpeed, "setMotorMoveSpeed");
    NLua::registerFunction(l, luaSetMotorDir, "setMotorDir");
    NLua::registerFunction(l, luaSetIRCOMMSendCallback, "setIRCOMMSendCallback");

    // LED class
    NLua::registerFunction(l, luaCreateLED, "createLED");
    NLua::registerClassFunction(l, luaLEDSetEnabled, "setEnabled", "led");

    // Bumper class
    NLua::registerFunction(l, luaCreateBumper, "createBumper");
    NLua::registerClassFunction(l, luaBumperSetCallback, "setCallback",
                                "bumper");

    // IR sensor class
    NLua::registerFunction(l, luaCreateIRSensor, "createIRSensor");
    NLua::registerClassFunction(l, luaIRSensorGetHitDistance, "getHitDistance",
                                "irsensor");

    // light sensor class
    NLua::registerFunction(l, luaCreateLightSensor, "createLightSensor");
    NLua::registerClassFunction(l, luaLightSensorGetLight, "getLight", "lightsensor");
}

void CRP6Simulator::registerLuaM32(lua_State *l)
{
    NLua::registerFunction(l, luaSetM32SerialSendCallback,
                           "setSerialSendCallback");
    NLua::registerFunction(l, luaAppendM32SerialOutput, "appendSerialOutput");
}

void CRP6Simulator::createMenus()
{
    QMenu *menu = menuBar()->addMenu("&File");
    menu->addAction("New", this, SLOT(newProject()), tr("ctrl+N"));
    menu->addAction(QIcon(style()->standardIcon(QStyle::SP_DialogOpenButton)),
                    "Open", this, SLOT(openProject()), tr("ctrl+O"));
    menu->addMenu("Open recent...")->addAction("blah");

    menu->addSeparator();

    menu->addAction("Quit", this, SLOT(close()), tr("ctrl+Q"));


    menu = menuBar()->addMenu("&Map");

    QAction *a = menu->addAction("New map", this, SLOT(newMap()));
    mapMenuActionList << a;

    a = menu->addAction(QIcon(style()->standardIcon(QStyle::SP_DialogSaveButton)),
                        "Save map", this, SLOT(saveMap()));
    connect(robotScene, SIGNAL(mapEditedChanged(bool)), a,
            SLOT(setEnabled(bool)));
    a->setEnabled(false);

    a = menu->addAction(QIcon(style()->standardIcon(QStyle::SP_DialogSaveButton)),
                        "Save map As...", this, SLOT(saveMapAs()));
    mapMenuActionList << a;
    a->setEnabled(false);

    a = menu->addAction(QIcon(style()->standardIcon(QStyle::SP_DialogOpenButton)),
                        "Load map", this, SLOT(loadMap()));
    mapMenuActionList << a;

    menu->addSeparator();


    menu = menuBar()->addMenu("&Edit");
    menu->addAction("Project settings");
    menu->addSeparator();
    menu->addAction("Preferences");


    menu = menuBar()->addMenu("&Help");
    menu->addAction("About");
    menu->addAction("About Qt", qApp, SLOT(aboutQt()));

    foreach (a, mapMenuActionList)
        a->setEnabled(false);
}

void CRP6Simulator::setToolBarToolTips()
{
    QList<QToolBar *> toolbars = findChildren<QToolBar *>();
    foreach (QToolBar *tb, toolbars)
    {
        QList<QAction *> acs = tb->actions();
        foreach (QAction *a, acs)
        {
            QString s = a->shortcut().toString();
            if (!s.isEmpty())
                a->setToolTip(QString("<qt>%1<br><small>%2</small></qt>")
                              .arg(a->toolTip()).arg(s));
        }
    }
}

void CRP6Simulator::createToolbars()
{
    QToolBar *toolb = addToolBar("Run");
    runPluginAction = toolb->addAction(QIcon(style()->standardIcon(QStyle::SP_MediaPlay)), "Run",
                                       this, SLOT(runPlugin()));
    runPluginAction->setShortcut(tr("ctrl+R"));
    runPluginAction->setEnabled(false);

    stopPluginAction = toolb->addAction(QIcon(style()->standardIcon(QStyle::SP_MediaStop)), "Stop",
                                        this, SLOT(stopPlugin()));
    stopPluginAction->setShortcut(tr("esc"));
    stopPluginAction->setEnabled(false);


    addToolBar(editMapToolBar = new QToolBar("Edit map"));
    editMapToolBar->setEnabled(false);

    editMapActionGroup = new QActionGroup(this);
    connect(editMapActionGroup, SIGNAL(triggered(QAction*)), this,
            SLOT(changeSceneMouseMode(QAction*)));

    QAction *a = editMapToolBar->addAction(QIcon("../resource/viewmag_.png"),
                                           "Zoom map out", robotScene,
                                           SLOT(zoomSceneOut()));
    a->setShortcut(QKeySequence("-"));

    a = editMapToolBar->addAction(QIcon("../resource/viewmag+.png"), "Zoom map in",
                                  robotScene, SLOT(zoomSceneIn()));
    a->setShortcut(QKeySequence("+"));

    a = editMapToolBar->addAction(QIcon("../resource/follow.png"),
                                  "Toggle robot following", robotScene,
                                  SLOT(setFollowRobot(bool)));
    connect(robotScene, SIGNAL(robotFollowingChanged(bool)), a,
            SLOT(setChecked(bool)));
    a->setCheckable(true);


    editMapToolBar->addSeparator();


    a = editMapToolBar->addAction(QIcon("../resource/edit-map.png"), "Edit map settings",
                                  this, SLOT(editMapSettings()));

    a = editMapToolBar->addAction(QIcon(style()->standardIcon(QStyle::SP_DialogSaveButton)),
                                  "Save map", this, SLOT(saveMap()));
    connect(robotScene, SIGNAL(mapEditedChanged(bool)), a,
            SLOT(setEnabled(bool)));

    a = editMapToolBar->addAction(QIcon("../resource/clear.png"),
                                  "Clear map (removes all walls and items)",
                                  robotScene, SLOT(clearMap()));


    editMapToolBar->addSeparator();


    a = editMapToolBar->addAction(QIcon("../resource/grid-icon.png"),
                                  "Toggle grid visibility",
                                  robotScene, SLOT(setGridVisible(bool)));
    a->setCheckable(true);

    a = editMapToolBar->addAction(QIcon("../resource/light-refresh.png"),
                                  "Update lighting", robotScene,
                                  SLOT(updateLighting()));

    QAction *lightedita =
            editMapToolBar->addAction(QIcon("../resource/light-mode.png"),
                                      "Toggle light edit mode",
                                      robotScene, SLOT(setLightEditMode(bool)));
    lightedita->setCheckable(true);


    editMapToolBar->addSeparator();

    a = editMapActionGroup->addAction(QIcon("../resource/mouse-arrow.png"),
                                      "Selection mode");
    a->setCheckable(true);
    a->setChecked(true);
    a->setData(CRobotScene::MODE_POINT);
    editMapToolBar->addAction(a);

    a = editMapActionGroup->addAction(QIcon("../resource/wall.jpg"), "Add wall");
    a->setCheckable(true);
    a->setData(CRobotScene::MODE_WALL);
    editMapToolBar->addAction(a);

    a = editMapActionGroup->addAction(QIcon("../resource/cardboard-box.png"),
                                      "Add box obstacle");
    a->setCheckable(true);
    a->setData(CRobotScene::MODE_BOX);
    editMapToolBar->addAction(a);

    a = editMapActionGroup->addAction(QIcon("../resource/light-add.png"),
                                      "Add light source");
    a->setCheckable(true);
    a->setEnabled(false);
    a->setData(CRobotScene::MODE_LIGHT);
    connect(lightedita, SIGNAL(toggled(bool)), a, SLOT(setEnabled(bool)));
    editMapToolBar->addAction(a);


    setToolBarToolTips();
}

QWidget *CRP6Simulator::createMainWidget()
{
    mainStackedWidget = new QStackedWidget;

    mainStackedWidget->addWidget(createProjectPlaceHolderWidget());

    mainStackedWidget->addWidget(projectTabWidget = new QTabWidget);

    // NOTE: createRobotWidget() needs to be called before
    //       createRobotSceneWidget()
    projectTabWidget->addTab(createRobotWidget(), "Robot");
    projectTabWidget->addTab(createRobotSceneWidget(), "Map");

    return mainStackedWidget;
}

QWidget *CRP6Simulator::createProjectPlaceHolderWidget()
{
    // Place holder when no project is loaded

    QWidget *ret = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout(ret);

    QLabel *label = new QLabel("<qt>Welcome to rp6sim!<br>"
                               "You can load/create projects from the "
                               "<b>File</b> menu.</qt>");
    label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    label->setAlignment(Qt::AlignCenter);
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->setMargin(15);
    vbox->addWidget(label, 0, Qt::AlignHCenter);

    return ret;
}

QWidget *CRP6Simulator::createRobotWidget()
{
    QWidget *ret = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout(ret);

    vbox->addWidget(robotWidget = new CRobotWidget);

    return ret;
}

QWidget *CRP6Simulator::createRobotSceneWidget()
{
    mapStackedWidget = new QStackedWidget;

    // Widget shown when no map is loaded
    QWidget *w = new QWidget;
    mapStackedWidget->addWidget(w);
    QVBoxLayout *vbox = new QVBoxLayout(w);

    QLabel *label = new QLabel("<qt>Currently no map is loaded.<br>"
                              "To load a map use the <b>Map</b> menu or "
                              "select a map or template from the "
                              "<b>Map selector</b>.</qt>");
    label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    label->setAlignment(Qt::AlignCenter);
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->setMargin(15);
    vbox->addWidget(label, 0, Qt::AlignHCenter);


    // Robot map scene widget shown when a map is loaded
    w = new QWidget;
    mapStackedWidget->addWidget(w);
    QHBoxLayout *hbox = new QHBoxLayout(w);

    robotScene = new CRobotScene(this);
    connect(robotScene, SIGNAL(mouseModeChanged(CRobotScene::EMouseMode)), this,
            SLOT(sceneMouseModeChanged(CRobotScene::EMouseMode)));
    connect(this, SIGNAL(motorDriveSpeedChanged(EMotor, int)),
            robotScene->getRobotItem(), SLOT(setMotorSpeed(EMotor, int)));
    connect(this, SIGNAL(motorDirectionChanged(EMotor, EMotorDirection)),
            robotScene->getRobotItem(),
            SLOT(setMotorDirection(EMotor, EMotorDirection)));
    connect(robotScene->getRobotItem(), SIGNAL(robotBlockedChanged(bool)),
            SLOT(setRobotIsBlocked(bool)));
    connect(robotScene->getRobotItem(), SIGNAL(bumperChanged(CBumper *, bool)),
            robotWidget, SLOT(update()));
    connect(robotScene->getRobotItem(), SIGNAL(bumperChanged(CBumper *, bool)),
            SLOT(setLuaBumper(CBumper *, bool)));
    connect(robotScene->getRobotItem(),
            SIGNAL(IRSensorChanged(CIRSensor *, float)), robotWidget,
            SLOT(update()));

    graphicsView = new QGraphicsView(robotScene);
    graphicsView->setRenderHints(QPainter::Antialiasing |
                                 QPainter::SmoothPixmapTransform);
    graphicsView->setMouseTracking(true);
    hbox->addWidget(graphicsView);

    vbox = new QVBoxLayout;
    hbox->addLayout(vbox);

    QSpinBox *spinbox = new QSpinBox;
    connect(spinbox, SIGNAL(valueChanged(int)), this,
            SLOT(debugSetRobotLeftPower(int)));
    spinbox->setRange(-210, 210);
    new QShortcut(QKeySequence("Q"), spinbox, SLOT(stepDown()));
    new QShortcut(QKeySequence("W"), spinbox, SLOT(stepUp()));
    vbox->addWidget(spinbox);

    spinbox = new QSpinBox;
    connect(spinbox, SIGNAL(valueChanged(int)), this,
            SLOT(debugSetRobotRightPower(int)));
    spinbox->setRange(-210, 210);
    new QShortcut(QKeySequence("A"), spinbox, SLOT(stepDown()));
    new QShortcut(QKeySequence("S"), spinbox, SLOT(stepUp()));
    vbox->addWidget(spinbox);

    return mapStackedWidget;
}

QWidget *CRP6Simulator::createBottomTabs()
{
    QTabWidget *ret = new QTabWidget;
    ret->setTabPosition(QTabWidget::South);
    ret->addTab(createLogTab(), "log");
    ret->addTab(createSerialRobotTab(), "serial - robot");
    ret->addTab(createSerialM32Tab(), "serial - m32");
    ret->addTab(createIRCOMMTab(), "IRCOMM");
    return ret;
}

QWidget *CRP6Simulator::createLogTab()
{
    logWidget = new QPlainTextEdit;
    logWidget->setReadOnly(true);
    return logWidget;
}

QWidget *CRP6Simulator::createSerialRobotTab()
{
    QWidget *ret = new QWidget;
    QGridLayout *grid = new QGridLayout(ret);

    grid->addWidget(robotSerialOutputWidget = new QPlainTextEdit, 0, 0, 1, 3);
    robotSerialOutputWidget->setReadOnly(true);

    grid->addWidget(robotSerialInputWidget = new QLineEdit, 1, 0);

    grid->addWidget(robotSerialSendButton = new QPushButton("Send"), 1, 1);
    robotSerialSendButton->setEnabled(false);
    connect(robotSerialSendButton, SIGNAL(clicked()), this,
            SLOT(sendRobotSerialText()));
    connect(robotSerialInputWidget, SIGNAL(returnPressed()),
            robotSerialSendButton, SLOT(click()));
    connect(this, SIGNAL(receivedRobotSerialSendCallback(bool)),
            robotSerialSendButton, SLOT(setEnabled(bool)));

    QPushButton *button = new QPushButton("Clear");
    connect(button, SIGNAL(clicked()), robotSerialOutputWidget, SLOT(clear()));
    grid->addWidget(button, 1, 2);

    return ret;
}

QWidget *CRP6Simulator::createSerialM32Tab()
{
    QWidget *ret = new QWidget;
    QGridLayout *grid = new QGridLayout(ret);

    grid->addWidget(m32SerialOutputWidget = new QPlainTextEdit, 0, 0, 1, 3);
    m32SerialOutputWidget->setReadOnly(true);

    grid->addWidget(m32SerialInputWidget = new QLineEdit, 1, 0);

    grid->addWidget(m32SerialSendButton = new QPushButton("Send"), 1, 1);
    m32SerialSendButton->setEnabled(false);
    connect(m32SerialSendButton, SIGNAL(clicked()), this,
            SLOT(sendM32SerialText()));
    connect(m32SerialInputWidget, SIGNAL(returnPressed()), m32SerialSendButton,
            SLOT(click()));
    connect(this, SIGNAL(receivedM32SerialSendCallback(bool)),
            m32SerialSendButton, SLOT(setEnabled(bool)));

    QPushButton *button = new QPushButton("Clear");
    connect(button, SIGNAL(clicked()), m32SerialOutputWidget, SLOT(clear()));
    grid->addWidget(button, 1, 2);

    return ret;
}

QWidget *CRP6Simulator::createIRCOMMTab()
{
    QWidget *ret = new QWidget;
    QGridLayout *grid = new QGridLayout(ret);

    grid->addWidget(IRCOMMOutputWidget = new QPlainTextEdit, 0, 0, 1, -1);
    IRCOMMOutputWidget->setReadOnly(true);
    connect(this, SIGNAL(newIRCOMMText(const QString &)), IRCOMMOutputWidget,
            SLOT(appendHtml(QString)));

    QLabel *l = new QLabel("Address");
    l->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    grid->addWidget(l, 1, 0);
    grid->addWidget(IRCOMMAddressWidget = new QSpinBox, 1, 1);
    IRCOMMAddressWidget->setRange(0, 31);

    grid->addWidget(l = new QLabel("Key code"), 1, 2);
    l->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    grid->addWidget(IRCOMMKeyWidget = new QSpinBox, 1, 3);
    IRCOMMKeyWidget->setRange(0, 63);

    grid->addWidget(l = new QLabel("Toggle bit"), 1, 4);
    l->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    grid->addWidget(IRCOMMToggleWidget = new QCheckBox, 1, 5);

    grid->addWidget(IRCOMMSendButton = new QPushButton("Send"), 1, 6);
    IRCOMMSendButton->setEnabled(false);
    connect(IRCOMMSendButton, SIGNAL(clicked()), SLOT(sendIRCOMM()));
    // UNDONE
//    connect(IRCOMMAddressWidget, SIGNAL(returnPressed()), IRCOMMSendButton,
//            SLOT(click()));
//    connect(IRCOMMKeyWidget, SIGNAL(returnPressed()), IRCOMMSendButton,
//            SLOT(click()));
//    connect(IRCOMMToggleWidget, SIGNAL(returnPressed()), IRCOMMSendButton,
//            SLOT(click()));
    connect(this, SIGNAL(receivedIRCOMMSendCallback(bool)), IRCOMMSendButton,
            SLOT(setEnabled(bool)));

    QPushButton *button = new QPushButton("Clear");
    grid->addWidget(button, 1, 7);
    connect(button, SIGNAL(clicked()), IRCOMMOutputWidget, SLOT(clear()));

    return ret;
}

QDockWidget *CRP6Simulator::createStatusDock()
{
    QDockWidget *ret = new QDockWidget("Status", this);
    ret->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QWidget *w = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout(w);
    ret->setWidget(w);

    QGroupBox *group = new QGroupBox("Robot");
    vbox->addWidget(group);
    QVBoxLayout *subvbox = new QVBoxLayout(group);

    subvbox->addWidget(robotStatusTreeWidget = new QTreeWidget);
    robotStatusTreeWidget->setHeaderLabels(QStringList() << "Device" << "Value");
    robotStatusTreeWidget->setSortingEnabled(true);
    robotStatusTreeWidget->sortByColumn(0, Qt::AscendingOrder);

    group = new QGroupBox("Map selector");
    vbox->addWidget(group);
    subvbox = new QVBoxLayout(group);

    subvbox->addWidget(mapSelectorTreeWidget = new QTreeWidget);
    mapSelectorTreeWidget->setHeaderHidden(true);
    mapSelectorTreeWidget->setSelectionMode(QTreeWidget::SingleSelection);
    mapSelectorTreeWidget->setEnabled(false);
    connect(mapSelectorTreeWidget, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
            SLOT(mapSelectorItemActivated(QTreeWidgetItem*)));

    mapTemplatesTreeItem =
            new QTreeWidgetItem(mapSelectorTreeWidget,
                                QStringList() << "Templates");
    mapHistoryTreeItem = new QTreeWidgetItem(mapSelectorTreeWidget,
                                             QStringList() << "Map history");
    mapHistoryTreeItem->setExpanded(true);
    loadMapTemplatesTree();
    loadMapHistoryTree();

    return ret;
}

QDockWidget *CRP6Simulator::createADCDock()
{
    QDockWidget *ret = new QDockWidget("ADC", this);
    ret->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QWidget *w = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout(w);
    ret->setWidget(w);

    QLabel *l =
            new QLabel("In this table you can override ADC values "
                       "that would otherwise be set by any of the "
                       "loaded drivers.");
    l->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    l->setWordWrap(true);
    vbox->addWidget(l);
    vbox->addSpacing(10);

    vbox->addWidget(ADCTableWidget = new QTableWidget(7, 2));
    ADCTableWidget->setHorizontalHeaderLabels(QStringList() << "override" <<
                                              "value");
    ADCTableWidget->setVerticalHeaderLabels(QStringList() << "ADC0" << "ADC1" <<
                                    "LS_L" << "LS_R" << "MCURRENT_L" <<
                                    "MCURRENT_R" << "BATTERY");

    for (int i=0; i<ADCTableWidget->rowCount(); ++i)
    {
        QWidget *cw = new QWidget;
        QHBoxLayout *chbox = new QHBoxLayout(cw);
        chbox->setAlignment(Qt::AlignCenter);
        chbox->setMargin(0);

        QCheckBox *check = new QCheckBox;
        chbox->addWidget(check);
        ADCOverrideCheckBoxes << check;
        ADCTableWidget->setCellWidget(i, 0, cw);

        QSpinBox *spin = new QSpinBox;
        spin->setRange(0, 1023); // UNDONE? ADC may have lower max
        spin->setEnabled(false);
        connect(check, SIGNAL(toggled(bool)), spin, SLOT(setEnabled(bool)));
        ADCOverrideSpinBoxes << spin;
        ADCTableWidget->setCellWidget(i, 1, spin);
    }

    ADCTableWidget->resizeRowsToContents();
    ADCTableWidget->resizeColumnToContents(0);

    QHBoxLayout *hbox = new QHBoxLayout;
    vbox->addLayout(hbox);

    QPushButton *button = new QPushButton("Reset");
    connect(button, SIGNAL(clicked()), SLOT(resetADCTable()));
    hbox->addWidget(button);

    hbox->addWidget(button = new QPushButton("Apply"));
    connect(button, SIGNAL(clicked()), SLOT(applyADCTable()));

    return ret;
}

QDockWidget *CRP6Simulator::createRegisterDock()
{
    extern const char *IORegisterStringArray[IO_END];

    QDockWidget *ret = new QDockWidget("Registers", this);
    ret->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QWidget *w = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout(w);
    ret->setWidget(w);

    vbox->addWidget(IORegisterTableWidget = new QTableWidget(IO_END, 3));
    IORegisterTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    IORegisterTableWidget->setAlternatingRowColors(true);
    IORegisterTableWidget->setHorizontalHeaderLabels(QStringList() << "DEC" << "HEX" << "BIN");
    QStringList rows;
    for (int i=0; i<IO_END; ++i)
    {
        rows << IORegisterStringArray[i];
        for (int j=0; j<3; ++j)
        {
            QTableWidgetItem *item = new QTableWidgetItem;
            item->setTextAlignment(Qt::AlignCenter);
            IORegisterTableWidget->setItem(i, j, item);
        }
    }
    IORegisterTableWidget->setVerticalHeaderLabels(rows);

    return ret;
}

void CRP6Simulator::updateMainStackedWidget()
{
    if (currentProjectFile.isEmpty())
        mainStackedWidget->setCurrentIndex(0);
    else
        mainStackedWidget->setCurrentIndex(1);
}

void CRP6Simulator::updateMapStackedWidget()
{
    if (currentMapFile.isEmpty())
        mapStackedWidget->setCurrentIndex(0);
    else
        mapStackedWidget->setCurrentIndex(1);

    editMapToolBar->setEnabled(!currentMapFile.isEmpty() &&
                               (projectTabWidget->currentIndex() == 1));
}

void CRP6Simulator::openProjectFile(const QString &file)
{
    CProjectSettings prsettings(file);

    if (!verifySettingsFile(prsettings))
        return;

    // UNDONE
    robotScene->getRobotItem()->setActiveM32Slot(SLOT_FRONT);

    prsettings.beginGroup("robot");
    if (!robotSimulator->loadProjectFile(prsettings))
        return;
    prsettings.endGroup();

    prsettings.beginGroup("m32");
    if (!m32Simulator->loadProjectFile(prsettings))
        return; // UNDONE: Close robot robot simulator?
    prsettings.endGroup();

    stopPluginAction->setEnabled(false);
    runPluginAction->setEnabled(true);

    foreach (QAction *a, mapMenuActionList)
        a->setEnabled(true);

    currentProjectFile = file;

    updateMainStackedWidget();
}

void CRP6Simulator::loadMapFile(const QString &file, bool istemplate)
{
    if (file == currentMapFile)
        return;

    QSettings s(file, QSettings::IniFormat);
    if (verifySettingsFile(s))
    {
        robotScene->loadMap(s);
        currentMapFile = file;
        updateMapStackedWidget();
        if (!istemplate)
            addMapHistoryFile(file);
        currentMapIsTemplate = istemplate;
    }
}

void CRP6Simulator::loadMapTemplatesTree()
{
    const QString templpath("../map_templates/"); // UNDONE
    QQueue<QPair<QString, QTreeWidgetItem *> > dirqueue;

    dirqueue.enqueue(qMakePair(templpath, (QTreeWidgetItem*)0));
    while (!dirqueue.isEmpty())
    {
        const QPair<QString, QTreeWidgetItem *> dpair(dirqueue.dequeue());
        qDebug() << "dirpath:" << dpair.first;
        QDirIterator dirit(dpair.first, QStringList() << "*.map",
                           QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
        QTreeWidgetItem *diritem = 0;
        while (dirit.hasNext())
        {
            const QString subpath(dirit.next());
            qDebug() << "subpath:" << subpath;

            if (!diritem)
            {
                if (!dpair.second)
                    diritem = mapTemplatesTreeItem;
                else
                {
                    const QString d(QDir(dpair.first).dirName());
                    diritem = new QTreeWidgetItem(dpair.second,
                                                  QStringList() << d);
                }
            }

            if (dirit.fileInfo().isDir())
                dirqueue.enqueue(qMakePair(subpath, diritem));
            else
            {
                QString f(dirit.fileName());
                f.remove(".map");
                QTreeWidgetItem *sit = new QTreeWidgetItem(diritem,
                                                           QStringList() << f);
                sit->setData(0, Qt::UserRole, subpath);
            }
        }
    }
}

bool CRP6Simulator::mapItemIsTemplate(QTreeWidgetItem *item) const
{
    QTreeWidgetItem *pit = item->parent();
    while (pit)
    {
        if (pit == mapTemplatesTreeItem)
            return true;
        pit = pit->parent();
    }
    return false;
}

void CRP6Simulator::syncMapHistoryTree(const QStringList &l)
{
    while (mapHistoryTreeItem->childCount())
        delete mapHistoryTreeItem->child(0);

    foreach (QString f, l)
        new QTreeWidgetItem(mapHistoryTreeItem, QStringList() << f);
}

void CRP6Simulator::loadMapHistoryTree()
{
    QSettings settings;
    QStringList historylist =
            settings.value("maphistory").toStringList();

    bool modified = false;
    QStringList::iterator it = historylist.begin();
    while (it != historylist.end())
    {
        if (!QFile::exists(*it))
        {
            modified = true;
            it = historylist.erase(it);
        }
        else
            ++it;
    }

    if (modified)
        settings.setValue("maphistory", historylist);

    syncMapHistoryTree(historylist);
}

void CRP6Simulator::addMapHistoryFile(const QString &file)
{
    const int max = 20; // UNDONE: Configurable?
    QSettings settings;
    QStringList historylist =
            settings.value("maphistory").toStringList();

    historylist.removeAll(file); // Remove any existing
    historylist.push_front(file);

    if (historylist.size() > max)
        historylist.pop_back();

    settings.setValue("maphistory", historylist);

    syncMapHistoryTree(historylist);

    const int size = mapHistoryTreeItem->childCount();
    for (int i=0; i<size; ++i)
    {
        QTreeWidgetItem *it = mapHistoryTreeItem->child(i);
        if (it->text(0) == file)
        {
            mapSelectorTreeWidget->setCurrentItem(it);
            break;
        }
    }
}

QString CRP6Simulator::getLogOutput(ELogType type, QString text) const
{
    text = Qt::escape(text);
    // Html doesn't like tabs too much
    text = text.replace('\t', QString("&nbsp;").repeated(4));
    text = text.replace('\n', "<br>");
    QString fs;
    switch (type)
    {
    case LOG_LOG: fs = text; break;
    case LOG_WARNING: fs = QString("<FONT color=#FF8040>WARNING:</FONT> %1").arg(text); break;
    case LOG_ERROR: fs = QString("<FONT color=#FF0000><strong>ERROR: </strong></FONT> %1").arg(text); break;
    }

    return QString("<FONT color=#0000FF><strong>[%1]</strong></FONT> %2")
            .arg(QTime::currentTime().toString()).arg(fs);
}

void CRP6Simulator::appendRobotStatusUpdate(const QStringList &strtree)
{
    QMutexLocker statlocker(&robotStatusMutex);

    // Check for earlier appearences
    const int size = strtree.size();
    bool present = false;
    for (QList<QStringList>::iterator it=robotStatusUpdateBuffer.begin();
         it!=robotStatusUpdateBuffer.end(); ++it)
    {
        const int othersize = it->size();
        if (size == othersize)
        {
            for (int i=0; i<(size-1); ++i)
            {
                present = (strtree[i] == (*it)[i]);
                if (!present)
                    break;
            }
            if (present)
            {
                (*it)[size-1] = strtree[size-1]; // Update value (ie last entry)
                break;
            }
        }
    }

    if (!present)
        robotStatusUpdateBuffer.append(strtree);
}

int CRP6Simulator::luaAppendLogOutput(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    QMutexLocker loglocker(&instance->logBufferMutex);

    const char *type = luaL_checkstring(l, 1);
    const char *text = luaL_checkstring(l, 2);

    ELogType t;
    if (!strcmp(type, "LOG"))
        t = LOG_LOG;
    else if (!strcmp(type, "WARNING"))
        t = LOG_WARNING;
    else if (!strcmp(type, "ERROR"))
        t = LOG_ERROR;
    else
        luaL_argerror(l, 1, "Wrong log type.");

    instance->logTextBuffer += instance->getLogOutput(t, text);

    return 0;
}

int CRP6Simulator::luaAppendRobotSerialOutput(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    QMutexLocker seriallocker(&instance->robotSerialBufferMutex);
    instance->robotSerialTextBuffer += luaL_checkstring(l, 1);
    return 0;
}

int CRP6Simulator::luaAppendM32SerialOutput(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    QMutexLocker seriallocker(&instance->m32SerialBufferMutex);
    instance->m32SerialTextBuffer += luaL_checkstring(l, 1);
    return 0;
}

int CRP6Simulator::luaSetDriverLists(lua_State *l)
{
    // This function should only be called by init(), where we do
    // not have to worry about threads

    luaL_checktype(l, 1, LUA_TTABLE); // Driver descriptions
    luaL_checktype(l, 2, LUA_TTABLE); // Default driver list

    QHash<QString, QVariant> list = NLua::convertLuaTable(l, 1);
    QHash<QString, QString> descmap;
    for(QHash<QString, QVariant>::iterator it=list.begin(); it!=list.end(); ++it)
        descmap[it.key()] = it.value().toString();

    instance->projectWizard->setDriverLists(descmap,
                                            NLua::getStringList(l, 2));
    return 0;
}

int CRP6Simulator::luaSetCmPerPixel(lua_State *l)
{
    // This function should only be called by init(), where we do
    // not have to worry about threads
    instance->robotScene->getRobotItem()->setCmPerPixel(luaL_checknumber(l, 1));
    return 0;
}

int CRP6Simulator::luaSetRobotLength(lua_State *l)
{
    // This function should only be called by init(), where we do
    // not have to worry about threads
    instance->robotScene->getRobotItem()->setRobotLength(luaL_checknumber(l, 1));
    return 0;
}

int CRP6Simulator::luaSetRobotM32Slot(lua_State *l)
{
    // This function should only be called by init(), where we do
    // not have to worry about threads
    const char *s = luaL_checkstring(l, 1);

    EM32Slot slot;
    if (!strcmp(s, "front"))
        slot = SLOT_FRONT;
    else if (!strcmp(s, "back"))
        slot = SLOT_BACK;

    // position
    luaL_checktype(l, 2, LUA_TTABLE);

    lua_rawgeti(l, 2, 1);
    const float x = luaL_checknumber(l, -1);
    lua_pop(l, 1);

    lua_rawgeti(l, 2, 2);
    const float y = luaL_checknumber(l, -1);
    lua_pop(l, 1);

    const float rot = luaL_checknumber(l, 3);

    instance->robotScene->getRobotItem()->setM32Slot(slot, QPointF(x, y), rot);
    return 0;
}

int CRP6Simulator::luaSetRobotM32Scale(lua_State *l)
{
    // This function should only be called by init(), where we do
    // not have to worry about threads
    instance->robotScene->getRobotItem()->setM32Scale(luaL_checknumber(l, 1));
    return 0;
}

int CRP6Simulator::luaLogIRCOMM(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    const int adr = luaL_checkint(l, 1);
    const int key = luaL_checkint(l, 2);
    const bool toggle = NLua::checkBoolean(l, 3);

    const QString text = QString("Received IRCOMM: "
                                 "Address: %1 | "
                                 "Key code: %2 | "
                                 "Toggle bit: %3\n").
            arg(adr).arg(key).arg(toggle);
    instance->emit newIRCOMMText(instance->getLogOutput(LOG_LOG, text));
    return 0;
}

int CRP6Simulator::luaUpdateRobotStatus(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    const int nargs = lua_gettop(l);
    QStringList strtree;

    for (int i=1; i<=nargs; ++i)
        strtree << luaL_checkstring(l, i);

    instance->appendRobotStatusUpdate(strtree);
    return 0;
}

int CRP6Simulator::luaRobotIsBlocked(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    lua_pushboolean(l, instance->robotIsBlocked);
    return 1;
}

int CRP6Simulator::luaCreateBumper(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);

    // points
    luaL_checktype(l, 1, LUA_TTABLE);
    int size = lua_objlen(l, 1);
    QPolygonF points;
    for (int i=1; i<=size; ++i)
    {
        // x, y pair
        lua_rawgeti(l, 1, i);
        luaL_checktype(l, -1, LUA_TTABLE);

        lua_rawgeti(l, -1, 1);
        const float x = luaL_checknumber(l, -1);
        lua_pop(l, 1);

        lua_rawgeti(l, -1, 2);
        const float y = luaL_checknumber(l, -1);
        lua_pop(l, 1);
        points << QPointF(x, y);
    }

    // color
    luaL_checktype(l, 2, LUA_TTABLE);

    lua_rawgeti(l, 2, 1);
    const int r = luaL_checkint(l, -1);
    lua_pop(l, 1);

    lua_rawgeti(l, 2, 2);
    const int g = luaL_checkint(l, -1);
    lua_pop(l, 1);

    lua_rawgeti(l, 2, 3);
    const int b = luaL_checkint(l, -1);
    lua_pop(l, 1);

    lua_rawgeti(l, 2, 4);
    const int a = luaL_optint(l, -1, 255);
    lua_pop(l, 1);

    CBumper *bumper = new CBumper(points, QColor(r, g, b, a));
    instance->robotScene->getRobotItem()->addBumper(bumper);
    instance->robotWidget->addBumper(bumper);
    NLua::createClass(l, bumper, "bumper", luaBumperDestr);
    qDebug() << "Created bumper";
    return 1;
}

int CRP6Simulator::luaBumperSetCallback(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    CBumper *b = NLua::checkClassData<CBumper>(l, 1, "bumper");
    luaL_checktype(l, 2, LUA_TFUNCTION);
    if (instance->bumperLuaCallbacks[b])
        luaL_unref(l, LUA_REGISTRYINDEX, instance->bumperLuaCallbacks[b]);
    lua_pushvalue(l, 2);
    instance->bumperLuaCallbacks[b] = luaL_ref(l, LUA_REGISTRYINDEX);
    return 0;
}

int CRP6Simulator::luaBumperDestr(lua_State *l)
{
    qDebug() << "Removing bumper";
    // NLua::CLuaLocker lualocker(l);
    CBumper *b = NLua::checkClassData<CBumper>(l, 1, "bumper");
    if (instance->bumperLuaCallbacks[b])
    {
        luaL_unref(l, LUA_REGISTRYINDEX, instance->bumperLuaCallbacks[b]);
        instance->bumperLuaCallbacks.remove(b);
    }
    instance->robotScene->getRobotItem()->removeBumper(b);
    instance->robotWidget->removeBumper(b);
    delete b;
    return 0;
}

int CRP6Simulator::luaCreateLED(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);

    // position
    luaL_checktype(l, 1, LUA_TTABLE);

    lua_rawgeti(l, 1, 1);
    const float x = luaL_checknumber(l, -1);
    lua_pop(l, 1);

    lua_rawgeti(l, 1, 2);
    const float y = luaL_checknumber(l, -1);
    lua_pop(l, 1);

    // color
    luaL_checktype(l, 2, LUA_TTABLE);

    lua_rawgeti(l, 2, 1);
    const int r = luaL_checkint(l, -1);
    lua_pop(l, 1);

    lua_rawgeti(l, 2, 2);
    const int g = luaL_checkint(l, -1);
    lua_pop(l, 1);

    lua_rawgeti(l, 2, 3);
    const int b = luaL_checkint(l, -1);
    lua_pop(l, 1);

    lua_rawgeti(l, 2, 4);
    const int a = luaL_optint(l, -1, 255);
    lua_pop(l, 1);

    const float rad = (float)luaL_checknumber(l, 3);

    CLED *led = new CLED(QPointF(x, y), QColor(r, g, b, a), rad);
    instance->robotScene->getRobotItem()->addLED(led);
    instance->robotWidget->addLED(led);
    NLua::createClass(l, led, "led", luaLEDDestr);
    return 1;
}

int CRP6Simulator::luaLEDSetEnabled(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    CLED *led = NLua::checkClassData<CLED>(l, 1, "led");
    QMutexLocker LEDlocker(&instance->robotLEDMutex);
    instance->changedLEDs[led] = NLua::checkBoolean(l, 2);
    return 0;
}

int CRP6Simulator::luaLEDDestr(lua_State *l)
{
    qDebug() << "Removing LED";
    // NLua::CLuaLocker lualocker(l);
    CLED *led = NLua::checkClassData<CLED>(l, 1, "led");
    instance->robotScene->getRobotItem()->removeLED(led);
    instance->robotWidget->removeLED(led);
    delete led;
    return 0;
}

int CRP6Simulator::luaSetMotorPower(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);

    const char *motor = luaL_checkstring(l, 1);
    const int power = luaL_checkint(l, 2);

    EMotor mtype;

    if (!strcmp(motor, "left"))
        mtype = MOTOR_LEFT;
    else if (!strcmp(motor, "right"))
        mtype = MOTOR_RIGHT;

#ifndef USEATOMIC
    QMutexLocker mlock(&instance->motorPowerMutex);
#endif
    instance->changedMotorPower[mtype] = power;

    return 0;
}

int CRP6Simulator::luaSetMotorDriveSpeed(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);

    const char *motor = luaL_checkstring(l, 1);
    const int speed = luaL_checkint(l, 2);

    EMotor mtype;

    if (!strcmp(motor, "left"))
        mtype = MOTOR_LEFT;
    else if (!strcmp(motor, "right"))
        mtype = MOTOR_RIGHT;

    instance->emit motorDriveSpeedChanged(mtype, speed);

    return 0;
}

int CRP6Simulator::luaSetMotorMoveSpeed(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);

    const char *motor = luaL_checkstring(l, 1);
    const int speed = luaL_checkint(l, 2);

    EMotor mtype;

    if (!strcmp(motor, "left"))
        mtype = MOTOR_LEFT;
    else if (!strcmp(motor, "right"))
        mtype = MOTOR_RIGHT;

#ifndef USEATOMIC
    QMutexLocker mlock(&instance->motorMoveSpeedMutex);
#endif
    instance->changedMotorMoveSpeed[mtype] = speed;

    return 0;
}

int CRP6Simulator::luaSetMotorDir(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);

    const char *ldir = luaL_checkstring(l, 1);
    const char *rdir = luaL_checkstring(l, 2);

    EMotorDirection ld, rd;

    if (!strcmp(ldir, "FWD"))
        ld = MOTORDIR_FWD;
    else if (!strcmp(ldir, "BWD"))
        ld = MOTORDIR_BWD;

    if (!strcmp(rdir, "FWD"))
        rd = MOTORDIR_FWD;
    else if (!strcmp(rdir, "BWD"))
        rd = MOTORDIR_BWD;

#ifndef USEATOMIC
    QMutexLocker mlock(&instance->motorDirectionMutex);
#endif
    instance->changedMotorDirection[MOTOR_LEFT] = ld;
    instance->changedMotorDirection[MOTOR_RIGHT] = rd;

    instance->emit motorDirectionChanged(MOTOR_LEFT, ld);
    instance->emit motorDirectionChanged(MOTOR_RIGHT, rd);

    return 0;
}

int CRP6Simulator::luaCreateIRSensor(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);

    // position
    luaL_checktype(l, 1, LUA_TTABLE);

    lua_rawgeti(l, 1, 1);
    const float x = luaL_checknumber(l, -1);
    lua_pop(l, 1);

    lua_rawgeti(l, 1, 2);
    const float y = luaL_checknumber(l, -1);
    lua_pop(l, 1);

    // color
    luaL_checktype(l, 2, LUA_TTABLE);

    lua_rawgeti(l, 2, 1);
    const int r = luaL_checkint(l, -1);
    lua_pop(l, 1);

    lua_rawgeti(l, 2, 2);
    const int g = luaL_checkint(l, -1);
    lua_pop(l, 1);

    lua_rawgeti(l, 2, 3);
    const int b = luaL_checkint(l, -1);
    lua_pop(l, 1);

    lua_rawgeti(l, 2, 4);
    const int a = luaL_optint(l, -1, 255);
    lua_pop(l, 1);

    const float rad = (float)luaL_checknumber(l, 3);
    const float angle = (float)luaL_checknumber(l, 4);
    const float dist = (float)luaL_checknumber(l, 5);

    CIRSensor *ir = new CIRSensor(QPointF(x, y), QColor(r, g, b, a),
                                  rad, angle, dist);
    instance->robotScene->getRobotItem()->addIRSensor(ir);
    instance->robotWidget->addIRSensor(ir);
    NLua::createClass(l, ir, "irsensor", luaIRSensorDestr);
    return 1;
}

int CRP6Simulator::luaIRSensorGetHitDistance(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    CIRSensor *ir = NLua::checkClassData<CIRSensor>(l, 1, "irsensor");
    lua_pushnumber(l, ir->getHitDistance());
    return 1;
}

int CRP6Simulator::luaIRSensorDestr(lua_State *l)
{
    qDebug() << "Removing IR sensor";
    // NLua::CLuaLocker lualocker(l);
    CIRSensor *ir = NLua::checkClassData<CIRSensor>(l, 1, "irsensor");
    instance->robotScene->getRobotItem()->removeIRSensor(ir);
    instance->robotWidget->removeIRSensor(ir);
    delete ir;
    return 0;
}

int CRP6Simulator::luaCreateLightSensor(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);

    // position
    luaL_checktype(l, 1, LUA_TTABLE);

    lua_rawgeti(l, 1, 1);
    const float x = luaL_checknumber(l, -1);
    lua_pop(l, 1);

    lua_rawgeti(l, 1, 2);
    const float y = luaL_checknumber(l, -1);
    lua_pop(l, 1);

    CLightSensor *light = new CLightSensor(QPointF(x, y));
    instance->robotScene->getRobotItem()->addLightSensor(light);
    NLua::createClass(l, light, "lightsensor", luaLightSensorDestr);
    return 1;
}

int CRP6Simulator::luaLightSensorGetLight(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    CLightSensor *light = NLua::checkClassData<CLightSensor>(l, 1, "lightsensor");
    lua_pushinteger(l, light->getLight());
    return 1;
}

int CRP6Simulator::luaLightSensorDestr(lua_State *l)
{
    qDebug() << "Removing light sensor";
    // NLua::CLuaLocker lualocker(l);
    CLightSensor *light = NLua::checkClassData<CLightSensor>(l, 1, "lightsensor");
    instance->robotScene->getRobotItem()->removeLightSensor(light);
    delete light;
    return 0;
}

int CRP6Simulator::luaSetRobotSerialSendCallback(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    luaL_checktype(l, 1, LUA_TFUNCTION);
    if (instance->robotSerialSendLuaCallback)
        luaL_unref(l, LUA_REGISTRYINDEX, instance->robotSerialSendLuaCallback);
    lua_pushvalue(l, 1);
    instance->robotSerialSendLuaCallback = luaL_ref(l, LUA_REGISTRYINDEX);
    instance->emit receivedRobotSerialSendCallback(true);
    return 0;
}

int CRP6Simulator::luaSetM32SerialSendCallback(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    luaL_checktype(l, 1, LUA_TFUNCTION);
    if (instance->m32SerialSendLuaCallback)
        luaL_unref(l, LUA_REGISTRYINDEX, instance->m32SerialSendLuaCallback);
    lua_pushvalue(l, 1);
    instance->m32SerialSendLuaCallback = luaL_ref(l, LUA_REGISTRYINDEX);
    instance->emit receivedM32SerialSendCallback(true);
    return 0;
}

int CRP6Simulator::luaSetIRCOMMSendCallback(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    luaL_checktype(l, 1, LUA_TFUNCTION);
    if (instance->IRCOMMSendLuaCallback)
        luaL_unref(l, LUA_REGISTRYINDEX, instance->IRCOMMSendLuaCallback);
    lua_pushvalue(l, 1);
    instance->IRCOMMSendLuaCallback = luaL_ref(l, LUA_REGISTRYINDEX);
    instance->emit receivedIRCOMMSendCallback(true);
    return 0;
}

void CRP6Simulator::handleRobotSerialDeviceData()
{
    if (robotSerialSendLuaCallback == 0)
        return;

    // UNDONE: Ignore data if not running(? callback == 0 in that case)

    QByteArray txt = handSerialDeviceData(robotSerialDevice,
                                          robotSimulator->getLuaState(),
                                          robotSerialSendLuaCallback);

    QTextCharFormat cf(robotSerialOutputWidget->currentCharFormat());
    cf.setForeground(Qt::lightGray);
    robotSerialOutputWidget->setCurrentCharFormat(cf);
    robotSerialOutputWidget->appendPlainText(txt);
}

void CRP6Simulator::handleM32SerialDeviceData()
{
    if (m32SerialSendLuaCallback == 0)
        return;

    // UNDONE: Ignore data if not running(? callback == 0 in that case)

    QByteArray txt = handSerialDeviceData(m32SerialDevice,
                                          m32Simulator->getLuaState(),
                                          m32SerialSendLuaCallback);

    QTextCharFormat cf(m32SerialOutputWidget->currentCharFormat());
    cf.setForeground(Qt::lightGray);
    m32SerialOutputWidget->setCurrentCharFormat(cf);
    m32SerialOutputWidget->appendPlainText(txt);
}

void CRP6Simulator::updateRobotClockDisplay(unsigned long hz)
{
    double mhz = static_cast<double>(hz) / 1000000.0;
    QString s = QString::number(mhz, 'g', 3);
    if ((s.length() < 3) && !s.contains('.'))
    {
        s += ".";
        s = s.leftJustified(4, '0');
    }

    appendRobotStatusUpdate(QStringList() << "robot" << "AVR" << "clock (MHz)" << s);
}

void CRP6Simulator::updateM32ClockDisplay(unsigned long hz)
{
    double mhz = static_cast<double>(hz) / 1000000.0;
    QString s = QString::number(mhz, 'g', 3);
    if ((s.length() < 3) && !s.contains('.'))
    {
        s += ".";
        s = s.leftJustified(4, '0');
    }

    appendRobotStatusUpdate(QStringList() << "m32" << "AVR" << "clock (MHz)" << s);
}

void CRP6Simulator::newProject()
{
    // UNDONE: Check if running

    // According to docs this should be called auto when show is called,
    // but it's not ...
    projectWizard->restart();
    if (projectWizard->exec() == QDialog::Accepted)
        openProjectFile(projectWizard->getProjectFile());
}

void CRP6Simulator::openProject()
{
    QString file =
            QFileDialog::getOpenFileName(this, "Open project file",
                                         QDir::homePath(),
                                         "RP6 simulator project files (*.rp6)");
    // UNDONE: Check if running
    if (!file.isEmpty())
        openProjectFile(file);
}

void CRP6Simulator::newMap()
{
    QDialog dialog(this);
    dialog.setModal(true);

    QFormLayout *form = new QFormLayout(&dialog);

    QComboBox *sizebox = new QComboBox;
    sizebox->addItems(QStringList() << "Small" << "Medium" << "Large");
    sizebox->setCurrentIndex(1);
    form->addRow("Map size", sizebox);

    CProjectSettings prsettings(currentProjectFile);
    QLineEdit *nameinput = new QLineEdit(prsettings.value("name").toString());
    form->addRow("Map name", nameinput);

    CPathInput *dirinput = new CPathInput("Select map destination directory",
                                          CPathInput::PATH_EXISTDIR,
                                          QFileInfo(currentProjectFile).path());
    form->addRow("Map directory", dirinput);

    QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                  QDialogButtonBox::Cancel);
    connect(bbox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(bbox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    form->addWidget(bbox);

    while (true)
    {
        if (dialog.exec() != QDialog::Accepted)
            break;

        const QString dir(dirinput->getPath());
        QFileInfo dinfo(dir);
        if (dir.isEmpty() || !dinfo.isDir() || !dinfo.isReadable() ||
            !dinfo.isWritable() || !dinfo.isExecutable())
        {
            QMessageBox::critical(this, "Invalid map directory specified",
                                  "The directory specified is invalid or you "
                                  "have insufficient access permissions.");
            continue;
        }

        const QString file(nameinput->text());
        if (file.isEmpty())
        {
            QMessageBox::critical(this, "Invalid map name specified",
                                  "Please specify a valid map name.");
            continue;
        }

        const QString path(QDir(dir).absoluteFilePath(file + ".map"));
        if (QFileInfo(path).isFile())
        {
            if (QMessageBox::warning(this, "Map file exists",
                                     "A map with the same name already exists in "
                                     "the directory specified. Overwrite?",
                                     QMessageBox::Yes | QMessageBox::No) ==
                    QMessageBox::No)
                continue;

            if (!QFile(path).remove())
            {
                QMessageBox::critical(this, "Remove file error",
                                      "Could not remove existing file!");
                break;
            }
        }

        QSettings s(path, QSettings::IniFormat);
        if (!verifySettingsFile(s))
            break;

        qreal size;
        switch (sizebox->currentIndex())
        {
        case 0: size = 500.0; break;
        default:
        case 1: size = 1500.0; break;
        case 2: size = 3000.0; break;
        }

        robotScene->newMap(s, QSizeF(size, size));
        loadMapFile(path, false);
        break;
    }
}

void CRP6Simulator::saveMap()
{
    Q_ASSERT(!currentMapFile.isEmpty());

    if (currentMapIsTemplate)
    {
        // If map is saved under another name it will be cleared
        // automatically from being a template.
        saveMapAs();
        return;
    }

    QSettings s(currentMapFile, QSettings::IniFormat);

    if (!verifySettingsFile(s))
        return;

    robotScene->saveMap(s);    
}

void CRP6Simulator::saveMapAs()
{
    Q_ASSERT(!currentMapFile.isEmpty());

    const QString projdir = QFileInfo(currentProjectFile).path();
    const QString destfile =
            QFileDialog::getSaveFileName(this, "Save map as", projdir,
                                         "map files (*.map)");
    if (destfile.isEmpty())
        return;

    QSettings s(destfile, QSettings::IniFormat);

    if (!verifySettingsFile(s))
        return;

    robotScene->saveMap(s);

    // Sets up correct file selector settings etc.
    loadMapFile(destfile, false);
}

void CRP6Simulator::loadMap()
{
    QString file =
            QFileDialog::getOpenFileName(this, "Open map file",
                                         QDir::homePath(),
                                         "RP6 map files (*.map)");
    // UNDONE: Check if running
    if (!file.isEmpty())
        loadMapFile(file, false);
}

void CRP6Simulator::timedUIUpdate()
{
#ifdef USEATOMIC
    const QAtomicInt *ioregisters = robotSimulator->getIORegisterArray();
#else
    QReadLocker iolocker(&simulator->getIORegisterLock());
    const TIORegisterData *ioregisters = simulator->getIORegisterArray();
#endif

    for (int i=0; i<IO_END; ++i)
    {
        TIORegisterData d = ioregisters[i]; // Shortcut
        IORegisterTableWidget->item(i, 0)->setText(QString::number(d));
        IORegisterTableWidget->item(i, 1)->setText(QString::number(d, 16));
        IORegisterTableWidget->item(i, 2)->setText(QString::number(d, 2).rightJustified(16, '0', true));
        IORegisterTableWidget->resizeRowToContents(i);
    }

    IORegisterTableWidget->resizeColumnsToContents();

    // UNDONE: Double check to avoid mutexes?

    QMutexLocker loglocker(&instance->logBufferMutex);
    if (!logTextBuffer.isEmpty())
    {
        logWidget->appendHtml(logTextBuffer);
        logTextBuffer.clear();
    }

    QMutexLocker rserialocker(&instance->robotSerialBufferMutex);
    if (!robotSerialTextBuffer.isEmpty())
    {
        // Append text, without adding a new paragraph
        QTextCursor cur = robotSerialOutputWidget->textCursor();
        cur.movePosition(QTextCursor::End);
        robotSerialOutputWidget->setTextCursor(cur);
        QTextCharFormat cf(robotSerialOutputWidget->currentCharFormat());
        cf.setForeground(Qt::black);
        robotSerialOutputWidget->setCurrentCharFormat(cf);
        robotSerialOutputWidget->insertPlainText(robotSerialTextBuffer);

        if (robotSerialDevice->isWritable())
            robotSerialDevice->write(robotSerialTextBuffer.toLatin1());

        robotSerialTextBuffer.clear();
    }

    QMutexLocker mserialocker(&instance->m32SerialBufferMutex);
    if (!m32SerialTextBuffer.isEmpty())
    {
        // Append text, without adding a new paragraph
        QTextCursor cur = m32SerialOutputWidget->textCursor();
        cur.movePosition(QTextCursor::End);
        m32SerialOutputWidget->setTextCursor(cur);
        QTextCharFormat cf(m32SerialOutputWidget->currentCharFormat());
        cf.setForeground(Qt::black);
        m32SerialOutputWidget->setCurrentCharFormat(cf);
        m32SerialOutputWidget->insertPlainText(m32SerialTextBuffer);

        if (m32SerialDevice->isWritable())
            m32SerialDevice->write(m32SerialTextBuffer.toLatin1());

        m32SerialTextBuffer.clear();
    }

    QMutexLocker statlocker(&robotStatusMutex);
    foreach (QStringList stritems, robotStatusUpdateBuffer)
    {
        QList<QTreeWidgetItem *> items =
                instance->robotStatusTreeWidget->findItems(stritems.first(), Qt::MatchFixedString | Qt::MatchCaseSensitive);

        QTreeWidgetItem *parent;

        if (!items.isEmpty())
        {
            Q_ASSERT(items.size() == 1);
            parent = items.first();
        }
        else
            parent = new QTreeWidgetItem(instance->robotStatusTreeWidget, QStringList() << stritems.first());

        const int size = stritems.size();
        for (int i=1;i<(size-1); ++i)
        {
            const int childcount = parent->childCount();
            QTreeWidgetItem *child = 0;

            for (int j=0; j<childcount; ++j)
            {
                if (parent->child(j)->text(0) == stritems[i])
                {
                    child = parent->child(j);
                    break;
                }
            }

            if (!child)
                child = new QTreeWidgetItem(parent, QStringList() << stritems[i]);

            parent = child;
        }

        parent->setText(1, stritems.last());
    }
    robotStatusUpdateBuffer.clear();

#ifndef USEATOMIC
    QMutexLocker mpowerlocker(&motorPowerMutex);
    QMutexLocker mspeedlocker(&motorMoveSpeedMutex);
#endif
    for (int i=0; i<MOTOR_END; ++i)
    {
        if (changedMotorPower[i] != -1)
            robotWidget->setMotorPower((EMotor)i, changedMotorPower[i]);
        if (changedMotorMoveSpeed[i] != -1)
            robotWidget->setMotorSpeed((EMotor)i, changedMotorMoveSpeed[i]);
        changedMotorPower[i] = changedMotorMoveSpeed[i] = -1;
    }

#ifndef USEATOMIC
    QMutexLocker mdirlocker(&motorDirectionMutex);
#endif
    for (int i=0; i<MOTORDIR_END; ++i)
    {
        if (changedMotorDirection[i] != -1)
        {
            robotWidget->setMotorDirection((EMotor)i,
                                           (EMotorDirection)(int)changedMotorDirection[i]);
            changedMotorDirection[i] = -1;
        }
    }
}

void CRP6Simulator::timedLEDUpdate()
{
    QMutexLocker LEDlocker(&robotLEDMutex);

    for (QMap<CLED *, bool>::iterator it=changedLEDs.begin();
         it!=changedLEDs.end(); ++it)
        it.key()->setEnabled(it.value());

    robotWidget->update();
    changedLEDs.clear();
}

void CRP6Simulator::runPlugin()
{
    if (!robotSimulator->runPlugin() || !m32Simulator->runPlugin())
        return;

    robotSerialOutputWidget->clear();
    robotStatusTreeWidget->clear();
    pluginUpdateUITimer->start();
    pluginUpdateLEDsTimer->start();

    runPluginAction->setEnabled(false);
    stopPluginAction->setEnabled(true);
    ADCDockWidget->setEnabled(true);
}

void CRP6Simulator::stopPlugin()
{
    if (robotSerialSendLuaCallback || IRCOMMSendLuaCallback ||
        !bumperLuaCallbacks.isEmpty())
    {
        // Need to do this before calling CSimulator::stopPlugin() to make sure
        // drivers can properly be unloaded

        NLua::CLuaLocker lualocker(robotSimulator->getLuaState());
        foreach (int ref, bumperLuaCallbacks)
            luaL_unref(robotSimulator->getLuaState(), LUA_REGISTRYINDEX, ref);
        bumperLuaCallbacks.clear();

        if (robotSerialSendLuaCallback)
        {
            luaL_unref(robotSimulator->getLuaState(), LUA_REGISTRYINDEX,
                       robotSerialSendLuaCallback);
            robotSerialSendLuaCallback = 0;
        }

        if (IRCOMMSendLuaCallback)
        {
            luaL_unref(robotSimulator->getLuaState(), LUA_REGISTRYINDEX,
                       IRCOMMSendLuaCallback);
            IRCOMMSendLuaCallback = 0;
        }
    }

    if (m32SerialSendLuaCallback)
    {
        NLua::CLuaLocker lualocker(m32Simulator->getLuaState());
        luaL_unref(m32Simulator->getLuaState(), LUA_REGISTRYINDEX,
                   m32SerialSendLuaCallback);
        m32SerialSendLuaCallback = 0;
    }

    robotSimulator->stopPlugin();
    m32Simulator->stopPlugin();

    pluginUpdateUITimer->stop();
    pluginUpdateLEDsTimer->stop();

    // Dump any remaining buffered data
    timedUIUpdate();
    timedLEDUpdate();

    runPluginAction->setEnabled(true);
    stopPluginAction->setEnabled(false);
    ADCDockWidget->setEnabled(false);

    robotSerialSendButton->setEnabled(false);
    IRCOMMSendButton->setEnabled(false);

    robotIsBlocked = false;
}

void CRP6Simulator::tabViewChanged(int index)
{
    const bool e = (index != 0);
    mapSelectorTreeWidget->setEnabled(e);
    editMapToolBar->setEnabled(e && !currentMapFile.isEmpty());
}

void CRP6Simulator::changeSceneMouseMode(QAction *a)
{
    const CRobotScene::EMouseMode mode =
            static_cast<CRobotScene::EMouseMode>(a->data().toInt());
    robotScene->setMouseMode(mode);
}

void CRP6Simulator::sceneMouseModeChanged(CRobotScene::EMouseMode mode)
{
    QList<QAction *> actions = editMapActionGroup->actions();
    foreach (QAction *a, actions)
        a->setChecked((a->data().toInt() == static_cast<int>(mode)));
}

void CRP6Simulator::editMapSettings()
{
    CMapSettingsDialog dialog;

    dialog.setMapSize(robotScene->sceneRect().size());
    dialog.setAutoRefreshLight(robotScene->getAutoRefreshLighting());
    dialog.setAmbientLight(robotScene->getAmbientLight());
    dialog.setShadowQuality(robotScene->getShadowQuality());
    dialog.setGridSize(robotScene->getGridSize());
    dialog.setAutoGridSnap(robotScene->getAutoGrid());

    if (dialog.exec() == QDialog::Accepted)
    {
        robotScene->setAutoRefreshLighting(dialog.getAutoRefreshLight());
        robotScene->setMapSize(dialog.getMapSize());
        qDebug() << "Ambient light changed:" << (robotScene->getAmbientLight() != dialog.getAmbientLight()) << robotScene->getAmbientLight() << dialog.getAmbientLight();
        robotScene->setAmbientLight(dialog.getAmbientLight());
        robotScene->setShadowQuality(dialog.getShadowQuality());
        robotScene->setGridSize(dialog.getGridSize());
        robotScene->setAutoGrid(dialog.getAutoGridSnap());
    }
}

void CRP6Simulator::mapSelectorItemActivated(QTreeWidgetItem *item)
{
    if (!item)
        return;

    if (item->parent() == mapHistoryTreeItem)
        loadMapFile(item->text(0), false);
    // Template map items without user data are subdirectories
    else if (mapItemIsTemplate(item) && !item->data(0, Qt::UserRole).isNull())
        loadMapFile(item->data(0, Qt::UserRole).toString(), true);
}

void CRP6Simulator::resetADCTable()
{
    // NOTE: setChecked() triggers ADCOverrideSpinBoxes[]->setEnabled()
    foreach (QCheckBox *box, ADCOverrideCheckBoxes)
        box->setChecked(false);
    applyADCTable();
}

void CRP6Simulator::applyADCTable()
{
    // UNDONE: m32
    NLua::CLuaLocker lualocker(robotSimulator->getLuaState());
    lua_getglobal(robotSimulator->getLuaState(), "setUIADCValue");
    const int findex = lua_gettop(robotSimulator->getLuaState());

    for (int i=0; i<ADCTableWidget->rowCount(); ++i)
    {
        lua_pushvalue(robotSimulator->getLuaState(), findex);
        lua_pushstring(robotSimulator->getLuaState(),
                       qPrintable(ADCTableWidget->verticalHeaderItem(i)->text()));
        if (ADCOverrideCheckBoxes[i]->isChecked())
            lua_pushinteger(robotSimulator->getLuaState(), ADCOverrideSpinBoxes[i]->value());
        else
            lua_pushnil(robotSimulator->getLuaState()); // nil: don't override
        lua_call(robotSimulator->getLuaState(), 2, 0);
    }
    lua_pop(robotSimulator->getLuaState(), 1); // Pop function
}

void CRP6Simulator::setLuaBumper(CBumper *b, bool e)
{
    NLua::CLuaLocker lualocker(robotSimulator->getLuaState());
    lua_rawgeti(robotSimulator->getLuaState(), LUA_REGISTRYINDEX,
                bumperLuaCallbacks[b]);
    lua_pushboolean(robotSimulator->getLuaState(), e);
    lua_call(robotSimulator->getLuaState(), 1, 0); // UNDONE: error handling
}

void CRP6Simulator::sendRobotSerialText()
{
    NLua::CLuaLocker lualocker(robotSimulator->getLuaState());
    lua_rawgeti(robotSimulator->getLuaState(), LUA_REGISTRYINDEX,
                robotSerialSendLuaCallback);
    lua_pushstring(robotSimulator->getLuaState(),
                   qPrintable(robotSerialInputWidget->text() + "\n"));
    lua_call(robotSimulator->getLuaState(), 1, 0);

    QTextCharFormat cf(robotSerialOutputWidget->currentCharFormat());
    cf.setForeground(Qt::red);
    robotSerialOutputWidget->setCurrentCharFormat(cf);
    robotSerialOutputWidget->appendPlainText(QString("> %1\n").arg(robotSerialInputWidget->text()));

    robotSerialInputWidget->clear();
}

void CRP6Simulator::sendM32SerialText()
{
    NLua::CLuaLocker lualocker(m32Simulator->getLuaState());
    lua_rawgeti(m32Simulator->getLuaState(), LUA_REGISTRYINDEX,
                m32SerialSendLuaCallback);
    lua_pushstring(m32Simulator->getLuaState(),
                   qPrintable(m32SerialInputWidget->text() + "\n"));
    lua_call(m32Simulator->getLuaState(), 1, 0);

    m32SerialOutputWidget->appendPlainText(QString("> %1\n").arg(m32SerialInputWidget->text()));

    m32SerialInputWidget->clear();
}

void CRP6Simulator::sendIRCOMM()
{
    NLua::CLuaLocker lualocker(robotSimulator->getLuaState());
    lua_rawgeti(robotSimulator->getLuaState(), LUA_REGISTRYINDEX,
                IRCOMMSendLuaCallback);
    lua_pushinteger(robotSimulator->getLuaState(), IRCOMMAddressWidget->value());
    lua_pushinteger(robotSimulator->getLuaState(), IRCOMMKeyWidget->value());
    lua_pushboolean(robotSimulator->getLuaState(), IRCOMMToggleWidget->isChecked());
    lua_call(robotSimulator->getLuaState(), 3, 0);

    const QString text = QString("Send IRCOMM: "
                                 "Address: %1 | "
                                 "Key code: %2 | "
                                 "Toggle bit: %3\n").
            arg(IRCOMMAddressWidget->value()).
            arg(IRCOMMKeyWidget->value()).
            arg(IRCOMMToggleWidget->isChecked());
    IRCOMMOutputWidget->appendHtml(getLogOutput(LOG_LOG, text));
}

void CRP6Simulator::debugSetRobotLeftPower(int power)
{
    robotScene->getRobotItem()->setMotorSpeed(MOTOR_LEFT, abs(power));
    if (power >= 0)
        robotScene->getRobotItem()->setMotorDirection(MOTOR_LEFT, MOTORDIR_FWD);
    else
        robotScene->getRobotItem()->setMotorDirection(MOTOR_LEFT, MOTORDIR_BWD);
}

void CRP6Simulator::debugSetRobotRightPower(int power)
{
    robotScene->getRobotItem()->setMotorSpeed(MOTOR_RIGHT, abs(power));
    if (power >= 0)
        robotScene->getRobotItem()->setMotorDirection(MOTOR_RIGHT, MOTORDIR_FWD);
    else
        robotScene->getRobotItem()->setMotorDirection(MOTOR_RIGHT, MOTORDIR_BWD);
}

bool CRP6Simulator::loadCustomDriverInfo(const QString &file, QString &name,
                                         QString &desc)
{
    lua_getglobal(robotSimulator->getLuaState(), "getDriver");
    lua_pushstring(robotSimulator->getLuaState(), qPrintable(file));

    lua_call(robotSimulator->getLuaState(), 1, 2);

    const bool ok = !(lua_isnil(robotSimulator->getLuaState(), -1));
    if (ok)
    {
        name = luaL_checkstring(robotSimulator->getLuaState(), -2);
        desc = luaL_checkstring(robotSimulator->getLuaState(), -1);
    }

    lua_pop(robotSimulator->getLuaState(), 2);

    return ok;
}
