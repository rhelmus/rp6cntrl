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

QTableWidget *createADCTableWidget(const QStringList &adclist)
{
    QTableWidget *ret = new QTableWidget(adclist.size(), 2);
    ret->setHorizontalHeaderLabels(QStringList() << "override" << "value");
    ret->setVerticalHeaderLabels(adclist);

    for (int i=0; i<ret->rowCount(); ++i)
    {
        QWidget *cw = new QWidget;
        QHBoxLayout *chbox = new QHBoxLayout(cw);
        chbox->setAlignment(Qt::AlignCenter);
        chbox->setMargin(0);

        QCheckBox *check = new QCheckBox;
        chbox->addWidget(check);
        ret->setCellWidget(i, 0, cw);

        QSpinBox *spin = new QSpinBox;
        spin->setRange(0, 1023); // UNDONE? ADC may have lower max
        spin->setEnabled(false);
        QObject::connect(check, SIGNAL(toggled(bool)), spin,
                         SLOT(setEnabled(bool)));
        ret->setCellWidget(i, 1, spin);
    }

    ret->resizeRowsToContents();
    ret->resizeColumnToContents(0);

    return ret;
}

QCheckBox *getADCTableCheckBox(QTableWidget *tablew, int row, int col)
{
    // NOTE: checkboxes are actually child widgets
    QWidget *w = tablew->cellWidget(row, col);
    QList<QCheckBox *> l(w->findChildren<QCheckBox *>());
    Q_ASSERT(l.size() == 1);
    return l[0];
}

void applyADCTable(lua_State *l, QTableWidget *tablew)
{
    NLua::CLuaLocker lualocker(l);
    lua_getglobal(l, "setUIADCValue");
    const int findex = lua_gettop(l);

    for (int i=0; i<tablew->rowCount(); ++i)
    {
        QSpinBox *spin = qobject_cast<QSpinBox *>(tablew->cellWidget(i, 1));

        lua_pushvalue(l, findex);
        lua_pushstring(l, qPrintable(tablew->verticalHeaderItem(i)->text()));

        if (spin->isEnabled())
            lua_pushinteger(l, spin->value());
        else
            lua_pushnil(l); // nil: don't override

        lua_call(l, 2, 0);
    }

    lua_pop(l, 1); // Pop function
}

QToolButton *createKeyButton(int key)
{
    const QString txt = QString::number(key);
    QToolButton *ret = new QToolButton;
    ret->setText(txt);
    ret->setToolTip(QString("M32 key pad button %1").arg(key));
    const QRect rect(ret->fontMetrics().boundingRect(txt));
    ret->setMaximumWidth(rect.width() + 15);
    ret->setMaximumHeight(rect.height() + 5);
    return ret;
}

QByteArray handleSerialDeviceData(QextSerialPort *dev, lua_State *l, int luacb)
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
    QMainWindow(parent), audioCurrentCycle(0.0), audioFrequency(0.0),
    beeperPitch(0), playingBeep(false), handClapMode(CLAP_NORMAL),
    playingHandClap(false), handClapSoundPos(0),
    extEEPROMMode(EXTEEPROMMODE_NUMERICAL), currentMapIsTemplate(false),
    robotIsBlocked(false), robotSerialSendLuaCallback(0),
    m32SerialSendLuaCallback(0), IRCOMMSendLuaCallback(0),
    luaHandleExtInt1Callback(0), luaHandleSoundCallback(0),
    luaHandleKeyPressCallback(0)
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

    QDockWidget *statdock, *regdock, *eepromdock;
    addDockWidget(Qt::LeftDockWidgetArea, statdock = createStatusDock(),
                  Qt::Vertical);
    addDockWidget(Qt::LeftDockWidgetArea, ADCDockWidget = createADCDock(),
                  Qt::Vertical);
    addDockWidget(Qt::LeftDockWidgetArea, regdock = createRegisterDock(),
                  Qt::Vertical);
    addDockWidget(Qt::LeftDockWidgetArea, eepromdock = createExtEEPROMDock(),
                  Qt::Vertical);
    tabifyDockWidget(statdock, ADCDockWidget);
    tabifyDockWidget(ADCDockWidget, regdock);
    tabifyDockWidget(regdock, eepromdock);
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

    initSDL();
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

    delete [] handClapCVT.buf;
    SDL_CloseAudio();
    SDL_Quit();
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
    else // UNDONE
        QMessageBox::critical(this, "Serial port error",
                              QString("Failed to open serial port:\n%1").arg(port->errorString()));
}

void CRP6Simulator::initSDL()
{
    handClapCVT.buf = 0;

    if (SDL_Init(SDL_INIT_AUDIO))
    {
        // UNDONE
        QMessageBox::critical(this, "SDL init error",
                              QString("Failed to initialize SDL:\n%1").arg(SDL_GetError()));
        return;
    }

    SDL_AudioSpec spec, retspec;

    spec.freq = AUDIO_SAMPLERATE;
    spec.format = AUDIO_S16SYS;
    spec.channels = 1;
    spec.samples = 512;
    spec.callback = SDLAudioCallback;

    if (SDL_OpenAudio(&spec, &retspec))
    {
        // UNDONE
        QMessageBox::critical(this, "Audio error",
                              QString("Failed to open audio device:\n%1").arg(SDL_GetError()));
        return;
    }

    // Based on http://www.libsdl.org/intro.en/usingsound.html
    SDL_AudioSpec wave;
    Uint8 *data;
    Uint32 dlen;

    if (SDL_LoadWAV("../resource/clapping-hands.wav", &wave, &data, &dlen) == NULL)
    {
        // UNDONE
        QMessageBox::critical(this, "Audio error",
                              QString("Failed to load clapping sound:\n%1").arg(SDL_GetError()));
        return;
    }

    SDL_BuildAudioCVT(&handClapCVT, wave.format, wave.channels, wave.freq,
                      AUDIO_S16SYS, 1, AUDIO_SAMPLERATE);
    handClapCVT.buf = new Uint8[dlen*handClapCVT.len_mult];
    memcpy(handClapCVT.buf, data, dlen);
    handClapCVT.len = dlen;
    SDL_ConvertAudio(&handClapCVT);
    SDL_FreeWAV(data);
}

void CRP6Simulator::initSimulators()
{
    robotSimulator = new CSimulator(this);
    connect(robotSimulator->getAVRClock(), SIGNAL(clockSpeed(unsigned long)),
            SLOT(updateRobotClockDisplay(unsigned long)));
    registerLuaGeneric(robotSimulator->getLuaState());
    registerLuaRobot(robotSimulator->getLuaState());
    robotSimulator->startLua("robot");
    robotDriverInfoList = getLuaDriverInfoList(robotSimulator->getLuaState());

    m32Simulator = new CSimulator(this);
    connect(m32Simulator->getAVRClock(), SIGNAL(clockSpeed(unsigned long)),
            SLOT(updateM32ClockDisplay(unsigned long)));
    registerLuaGeneric(m32Simulator->getLuaState());
    registerLuaM32(m32Simulator->getLuaState());
    m32Simulator->startLua("m32");
    m32DriverInfoList = getLuaDriverInfoList(m32Simulator->getLuaState());

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

    NLua::registerClassFunction(l, luaLEDSetEnabled, "setEnabled", "led");
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
    NLua::registerFunction(l, luaSetExtInt1Enabled, "setExtInt1Enabled");

    // LED class
    NLua::registerFunction(l, luaCreateLED, "createLED", (void *)"robot");

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
    NLua::registerFunction(l, luaSetExtInt1Callback, "setExtInt1Callback");
    NLua::registerFunction(l, luaSetExtEEPROM, "setExtEEPROM");
    NLua::registerFunction(l, luaGetExtEEPROM, "getExtEEPROM");
    NLua::registerFunction(l, luaSetBeeperEnabled, "setBeeperEnabled");
    NLua::registerFunction(l, luaSetBeeperFrequency, "setBeeperFrequency");
    NLua::registerFunction(l, luaSetSoundCallback, "setSoundCallback");
    NLua::registerFunction(l, luaSetKeyPressCallback, "setKeyPressCallback");

    // LED class
    NLua::registerFunction(l, luaCreateLED, "createLED", (void *)"m32");
}

TDriverInfoList CRP6Simulator::getLuaDriverInfoList(lua_State *l)
{
    TDriverInfoList ret;

    lua_getglobal(l, "getDriverInfoList");
    lua_call(l, 0, 1);

    luaL_checktype(l, -1, LUA_TTABLE);
    const int tabind = NLua::luaAbsIndex(l, -1);
    const int size = lua_objlen(l, 1);

    for (int i=1; i<=size; ++i)
    {
        lua_rawgeti(l, tabind, i);
        luaL_checktype(l, -1, LUA_TTABLE);

        lua_getfield(l, -1, "name");
        const char *name = luaL_checkstring(l, -1);
        lua_pop(l, 1);

        lua_getfield(l, -1, "description");
        const char *desc = luaL_checkstring(l, -1);
        lua_pop(l, 1);

        lua_getfield(l, -1, "default");
        const bool def = NLua::checkBoolean(l, -1);
        lua_pop(l, 1);

        lua_pop(l, 1); // driver info table

        ret << SDriverInfo(name, desc, def);
    }

    return ret;
}

void CRP6Simulator::createMenus()
{
    QMenu *menu = menuBar()->addMenu("&File");
    menu->addAction("New Project", this, SLOT(newProject()), tr("ctrl+N"));
    menu->addAction(QIcon(style()->standardIcon(QStyle::SP_DialogOpenButton)),
                    "Open Project", this, SLOT(openProject()), tr("ctrl+O"));
    menu->addMenu("Open recent...")->addAction("blah"); // UNDONE

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

    saveMapAsAction = menu->addAction(QIcon(style()->standardIcon(QStyle::SP_DialogSaveButton)),
                                      "Save map As...", this, SLOT(saveMapAs()));
    mapMenuActionList << saveMapAsAction;
    saveMapAsAction->setEnabled(false);

    a = menu->addAction(QIcon(style()->standardIcon(QStyle::SP_DialogOpenButton)),
                        "Load map", this, SLOT(loadMap()));
    mapMenuActionList << a;

    menu->addSeparator();


    menu = menuBar()->addMenu("&Edit");
    editProjectSettingsAction =  menu->addAction("Project settings", this,
                                                 SLOT(editProjectSettings()));
    editProjectSettingsAction->setEnabled(false);

    menu->addSeparator();

    menu->addAction("Preferences");


    menu = menuBar()->addMenu("&Help");
    menu->addAction("About", this, SLOT(showAbout()));
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
    runPluginAction = toolb->addAction(QIcon("../resource/run.png"), "Run",
                                       this, SLOT(runPlugin()));
    runPluginAction->setShortcut(tr("ctrl+R"));
    runPluginAction->setEnabled(false);

    stopPluginAction = toolb->addAction(QIcon("../resource/stop.png"), "Stop",
                                        this, SLOT(stopPlugin()));
    stopPluginAction->setShortcut(tr("esc"));
    stopPluginAction->setEnabled(false);

    resetPluginAction = toolb->addAction(QIcon("../resource/reset.png"), "Reset");
    connect(resetPluginAction, SIGNAL(triggered()), SLOT(stopPlugin()));
    connect(resetPluginAction, SIGNAL(triggered()), SLOT(runPlugin()));
    resetPluginAction->setEnabled(false);

    toolb->addSeparator();

    QSignalMapper *signalm = new QSignalMapper(this);
    connect(signalm, SIGNAL(mapped(int)), SLOT(setHandClapMode(int)));
    toolb->addWidget(handClapButton = new QToolButton);
    handClapButton->setEnabled(false);
    handClapButton->setIcon(QIcon("../resource/clapping-hands.png"));
    handClapButton->setText("normal");
    handClapButton->setToolTip("Clap hands with selected volume (triggers MIC)");
    handClapButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    connect(handClapButton, SIGNAL(clicked(bool)), SLOT(doHandClap()));
    connect(this, SIGNAL(receivedSoundCallback(bool)), handClapButton,
            SLOT(setEnabled(bool)));
    QMenu *menu = new QMenu("Clap");
    handClapButton->setMenu(menu);
    QAction *a = menu->addAction("Soft", signalm, SLOT(map()));
    signalm->setMapping(a, CLAP_SOFT);
    a = menu->addAction("Normal (default)", signalm, SLOT(map()));
    signalm->setMapping(a, CLAP_NORMAL);
    a = menu->addAction("Loud", signalm, SLOT(map()));
    signalm->setMapping(a, CLAP_LOUD);

    signalm = new QSignalMapper(this);
    connect(signalm, SIGNAL(mapped(int)), SLOT(handleKeyPadPress(int)));
    toolb->addWidget(keyPadWidget = new QWidget);
    connect(this, SIGNAL(receivedKeyPressCallback(bool)), keyPadWidget,
            SLOT(setEnabled(bool)));
    keyPadWidget->setEnabled(false);
    QGridLayout *grid = new QGridLayout(keyPadWidget);
    for (int i=1; i<=5; ++i)
    {
        QToolButton *b = createKeyButton(i);
        connect(b, SIGNAL(pressed()), signalm, SLOT(map()));
        signalm->setMapping(b, i);
        connect(b, SIGNAL(released()), SLOT(resetKeyPress()));

        // Wrap after button 3
        const int row = (i > 3) ? 1 : 0;
        const int col = (i > 3) ? (i-4) : (i-1);
        grid->addWidget(b, row, col);
    }


    robotToolBar = addToolBar("Robot");
    robotToolBar->setEnabled(false);
    QToolButton *tb = new QToolButton;
    tb->setIcon(QIcon("../resource/dataplot.png"));
    robotToolBar->addWidget(tb);
    tb->setMenu(dataPlotMenu = new QMenu);
    tb->setPopupMode(QToolButton::InstantPopup);
    a = dataPlotMenu->addAction("Motor Speed");
    a->setData(CRobotWidget::DATAPLOT_MOTORSPEED);
    a = dataPlotMenu->addAction("Motor Power");
    a->setData(CRobotWidget::DATAPLOT_MOTORPOWER);
    a = dataPlotMenu->addAction("Light");
    a->setData(CRobotWidget::DATAPLOT_LIGHT);
    a = dataPlotMenu->addAction("Piezo");
    a->setData(CRobotWidget::DATAPLOT_PIEZO);
    a = dataPlotMenu->addAction("Microphone");
    a->setData(CRobotWidget::DATAPLOT_MIC);
    a = dataPlotMenu->addAction("ADC - robot");
    a->setData(CRobotWidget::DATAPLOT_ROBOTADC);
    a = dataPlotMenu->addAction("ADC - m32");
    a->setData(CRobotWidget::DATAPLOT_M32ADC);
    connect(dataPlotMenu, SIGNAL(triggered(QAction*)),
            SLOT(handleDataPlotSelection(QAction*)));


    addToolBar(editMapToolBar = new QToolBar("Edit map"));
    editMapToolBar->setEnabled(false);

    editMapActionGroup = new QActionGroup(this);
    connect(editMapActionGroup, SIGNAL(triggered(QAction*)), this,
            SLOT(changeSceneMouseMode(QAction*)));

    a = editMapToolBar->addAction(QIcon("../resource/viewmag_.png"),
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
    connect(robotWidget, SIGNAL(dataPlotClosed(int)),
            SLOT(enableDataPlotSelection(int)));
    connect(this, SIGNAL(beeperPitchChanged(int)), robotWidget,
            SLOT(setBeeperPitch(int)));
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

    QGroupBox *groupbox = new QGroupBox("RP6");
    vbox->addWidget(groupbox);
    QVBoxLayout *subvbox = new QVBoxLayout(groupbox);
    robotADCTableWidget =
            createADCTableWidget(QStringList() << "ADC0" << "ADC1" <<
                                 "LS_L" << "LS_R" << "MCURRENT_L" <<
                                 "MCURRENT_R" << "BATTERY");
    subvbox->addWidget(robotADCTableWidget);

    vbox->addWidget(groupbox = new QGroupBox("m32"));
    subvbox = new QVBoxLayout(groupbox);
    m32ADCTableWidget =
            createADCTableWidget(QStringList() << "MIC" << "KEYPAD" <<
                                 "ADC2" << "ADC3" << "ADC4" << "ADC5" <<
                                 "ADC6" << "ADC7");
    subvbox->addWidget(m32ADCTableWidget);


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

    vbox->addWidget(IORegisterTableSelector = new QComboBox);
    IORegisterTableSelector->addItems(QStringList() << "RP6" << "m32");

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

QDockWidget *CRP6Simulator::createExtEEPROMDock()
{
    QDockWidget *ret = new QDockWidget("Ext. EEPROM", this);
    ret->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QWidget *w = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout(w);
    ret->setWidget(w);

    QWidget *subw = new QWidget;
    subw->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    vbox->addWidget(subw, 0, Qt::AlignTop);
    QGridLayout *grid = new QGridLayout(subw);


    QLabel *l = new QLabel("The table belows shows the contents "
                           "of the currently selected external EEPROM page "
                           "(left to right, top to bottom). Selecting a "
                           "cell allows you to modify a single byte value.");
    l->setWordWrap(true);
    grid->addWidget(l, 0, 0, 1, 3);

    grid->addItem(new QSpacerItem(0, 10), 1, 0);

    grid->addWidget(extEEPROMModeComboBox = new QComboBox, 2, 0);
    extEEPROMModeComboBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    extEEPROMModeComboBox->addItems(QStringList() << "Numerical" << "Character");
    extEEPROMModeComboBox->setToolTip("Display/edit mode");
    connect(extEEPROMModeComboBox, SIGNAL(currentIndexChanged(int)),
            SLOT(updateExtEEPROMMode(int)));

    grid->addWidget(extEEPROMPageSpinBox = new QSpinBox, 2, 2);
    extEEPROMPageSpinBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    extEEPROMPageSpinBox->setToolTip("Ext. EEPROM page");
    extEEPROMPageSpinBox->setRange(0, (EXTEEPROM_CAPACITY/EXTEEPROM_PAGESIZE)-1);
    connect(extEEPROMPageSpinBox, SIGNAL(valueChanged(int)),
            SLOT(updateExtEEPROM(int)));

    grid->addWidget(extEEPROMTableWidget = new QTableWidget(8, 8), 3, 0, 1, 4);
    extEEPROMTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    extEEPROMTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    extEEPROMTableWidget->verticalHeader()->hide();
    extEEPROMTableWidget->horizontalHeader()->hide();
    extEEPROMTableWidget->setSizePolicy(QSizePolicy::Preferred,
                                        QSizePolicy::Maximum);
    extEEPROMTableWidget->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    extEEPROMTableWidget->verticalHeader()->setResizeMode(QHeaderView::Stretch);
    // Avoid unselected state, do this before connecting the signal!
    extEEPROMTableWidget->setCurrentCell(0, 0);
    connect(extEEPROMTableWidget,
            SIGNAL(currentItemChanged(QTableWidgetItem*,QTableWidgetItem*)),
            SLOT(updateExtEEPROMSelection(QTableWidgetItem*)));
    for (int i=0; i<EXTEEPROM_TABLEDIMENSION; ++i)
    {
        for (int j=0; j<EXTEEPROM_TABLEDIMENSION; ++j)
        {
            QTableWidgetItem *item = new QTableWidgetItem;
            item->setTextAlignment(Qt::AlignCenter);
            item->setText("255"); // Uninitialized value
            extEEPROMTableWidget->setItem(i, j, item);
        }
    }

    grid->addWidget(extEEPROMInputStackedWidget = new QStackedWidget, 4, 0);
    extEEPROMInputStackedWidget->setSizePolicy(QSizePolicy::Fixed,
                                               QSizePolicy::Fixed);

    extEEPROMInputStackedWidget->addWidget(extEEPROMInputSpinBox = new QSpinBox);
    extEEPROMInputSpinBox->setRange(0, 255);

    extEEPROMInputStackedWidget->addWidget(extEEPROMInputLineEdit = new QLineEdit);
    extEEPROMInputLineEdit->setInputMask("N"); // 1 Alpha-Numeric character

    extEEPROMInputStackedWidget->setCurrentIndex(0);

    QPushButton *button = new QPushButton("Apply");
    connect(button, SIGNAL(clicked()), SLOT(applyExtEEPROMChange()));
    grid->addWidget(button, 4, 1);

    // Connect to self: force code to main thread
    connect(this, SIGNAL(extEEPROMByteChanged(int,quint8)),
            SLOT(updateExtEEPROMByte(int,quint8)));

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
    robotScene->getRobotItem()->setActiveM32Slot(SLOT_BACK);
    robotWidget->setActiveM32Slot(SLOT_BACK);

    prsettings.beginGroup("robot");
    if (!robotSimulator->loadProjectFile(prsettings))
        return;
    prsettings.endGroup();

    prsettings.beginGroup("m32");
    if (!m32Simulator->loadProjectFile(prsettings))
        return; // UNDONE: Close robot simulator?
    prsettings.endGroup();

    stopPluginAction->setEnabled(false);
    runPluginAction->setEnabled(true);
    resetPluginAction->setEnabled(false);
    robotToolBar->setEnabled(true);

    foreach (QAction *a, mapMenuActionList)
        a->setEnabled(true);
    saveMapAsAction->setEnabled(!currentMapFile.isNull());
    editProjectSettingsAction->setEnabled(true);

    currentProjectFile = file;

    updateMainStackedWidget();
    updateExtEEPROM(extEEPROMPageSpinBox->value());
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
        saveMapAsAction->setEnabled(true);
    }
}

bool CRP6Simulator::checkMapChange()
{
    if (robotScene->getMapEdited())
    {
        int but = QMessageBox::question(this, "Map changed",
                                  "The current map was changed. "
                                  "Do you want to save it?",
                                  QMessageBox::Yes | QMessageBox::No |
                                  QMessageBox::Cancel,
                                  QMessageBox::Yes);
        if (but == QMessageBox::Yes)
            return saveMap();
        else if (but == QMessageBox::No)
            return true;

        return false; // Cancelled
    }

    return true;
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
    const int max = 20;
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

QVariantList CRP6Simulator::getExtEEPROM() const
{
    CProjectSettings prsettings(currentProjectFile);
    if (!verifySettingsFile(prsettings))
        return QVariantList();

    prsettings.beginGroup("m32");
    QVariantList eeprom;

    if (prsettings.contains("exteeprom"))
        return prsettings.value("exteeprom").toList();

    // Initialize EEPROM with 255 as value for each byte
    QVariantList ret;
    for (int i=0; i<EXTEEPROM_CAPACITY; ++i)
        ret << 255;

    prsettings.setValue("exteeprom", ret); // Cache

    return ret;
}

QVariantHash CRP6Simulator::getADCValues(lua_State *l)
{
    NLua::CLuaLocker lualocker(l);

    lua_getglobal(l, "getADCValues");
    lua_call(l, 0, 1);
    luaL_checktype(l, -1, LUA_TTABLE);

    QVariantHash ret(NLua::convertLuaTable(l, -1));
    lua_pop(l, 1);

    return ret;
}

void CRP6Simulator::SDLAudioCallback(void *, Uint8 *stream, int length)
{
    if (instance->playingHandClap)
    {
        const int amount =
                qMin(length, (instance->handClapCVT.len_cvt-instance->handClapSoundPos));

        int volume = SDL_MIX_MAXVOLUME;
        if (instance->handClapMode == CLAP_NORMAL)
            volume *= 0.75;
        else if (instance->handClapMode == CLAP_SOFT)
            volume *= 0.5;

        SDL_MixAudio(stream, &instance->handClapCVT.buf[instance->handClapSoundPos],
                     amount, volume);
        instance->handClapSoundPos += amount;

        // Done?
        if (instance->handClapSoundPos >= instance->handClapCVT.len)
        {
            instance->playingHandClap = false;
            if (!instance->playingBeep)
                SDL_PauseAudio(1);
        }
    }
    else
    {
        // Based on http://www.dgames.org/beep-sound-with-sdl/

        // Convert to 16 bit stream
        Sint16 *stream16 = (Sint16 *)stream;
        int length16 = length / 2;

        // Non-aliased square wave, code taken from Audacity (effects/ToneGen.cpp)
        const double pre2PI = 2 * M_PI;
        const double pre4divPI = 4. / M_PI;

        for(int i=0; i<length16; ++i)
        {
            // Scaling
            const float b = (1. + cos((pre2PI * instance->audioFrequency)/AUDIO_SAMPLERATE))/pre4divPI;
            float f = pre4divPI *
                    sin(pre2PI * instance->audioCurrentCycle / AUDIO_SAMPLERATE);
            for(int k=3; (k<200) && (k * instance->audioFrequency < AUDIO_SAMPLERATE/2.); k+=2)
            {
                // Hanning Window in freq domain
                const float a = 1. + cos((pre2PI * k * instance->audioFrequency)/AUDIO_SAMPLERATE);
                // calc harmonic, apply window, scale to amplitude of fundamental
                f += a * sin(pre2PI * instance->audioCurrentCycle/AUDIO_SAMPLERATE * k)/(b*k);
            }

            // UNDONE: configurable amplitude (volume)
            stream16[i] = 32760/5.0 * f;
            instance->audioCurrentCycle += instance->audioFrequency;
        }
    }
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

    QVariantHash list = NLua::convertLuaTable(l, 1);
    QHash<QString, QString> descmap;
    for(QVariantHash::iterator it=list.begin(); it!=list.end(); ++it)
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
    instance->robotWidget->setM32Slot(slot, QPointF(x, y), rot);
    return 0;
}

int CRP6Simulator::luaSetRobotM32Scale(lua_State *l)
{
    // This function should only be called by init(), where we do
    // not have to worry about threads
    instance->robotScene->getRobotItem()->setM32Scale(luaL_checknumber(l, 1));
    instance->robotWidget->setM32Scale(luaL_checknumber(l, 1));
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

        lua_pop(l, 1); // pair table
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

    const char *board = NLua::getFromClosure<const char *>(l);

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

    if (!strcmp(board, "robot"))
    {
        instance->robotScene->getRobotItem()->addRobotLED(led);
        instance->robotWidget->addRobotLED(led);
    }
    else
    {
        instance->robotScene->getRobotItem()->addM32LED(led);
        instance->robotWidget->addM32LED(led);
    }

    NLua::createClass(l, led, "led", luaLEDDestr, (void *)board);
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
    const char *board = NLua::getFromClosure<const char *>(l);
    CLED *led = NLua::checkClassData<CLED>(l, 1, "led");

    if (!strcmp(board, "robot"))
    {
        instance->robotScene->getRobotItem()->removeRobotLED(led);
        instance->robotWidget->removeRobotLED(led);
    }
    else
    {
        instance->robotScene->getRobotItem()->removeM32LED(led);
        instance->robotWidget->removeM32LED(led);
    }

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

int CRP6Simulator::luaSetExtInt1Callback(lua_State *l)
{
    luaL_checktype(l, 1, LUA_TFUNCTION);
    if (instance->luaHandleExtInt1Callback)
        luaL_unref(l, LUA_REGISTRYINDEX, instance->luaHandleExtInt1Callback);
    lua_pushvalue(l, 1);
    instance->luaHandleExtInt1Callback = luaL_ref(l, LUA_REGISTRYINDEX);

    return 0;
}

int CRP6Simulator::luaSetExtInt1Enabled(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);

    if (instance->luaHandleExtInt1Callback != 0)
    {
        const bool e = NLua::checkBoolean(l, 1);
        NLua::CLuaLocker lock(instance->m32Simulator->getLuaState());
        lua_rawgeti(instance->m32Simulator->getLuaState(), LUA_REGISTRYINDEX,
                    instance->luaHandleExtInt1Callback);
        lua_pushboolean(instance->m32Simulator->getLuaState(), e);
        lua_call(instance->m32Simulator->getLuaState(), 1, 0);
    }

    return 0;
}

int CRP6Simulator::luaSetExtEEPROM(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);

    const int adr = luaL_checkint(l, 1);
    const uint8_t data = luaL_checkint(l, 2);

    QVariantList eeprom(instance->getExtEEPROM());

    if (eeprom.isEmpty())
        return 0; // Something went wrong

    eeprom[adr] = data;
    instance->emit extEEPROMByteChanged(adr, data);

    CProjectSettings prsettings(instance->currentProjectFile);
    if (verifySettingsFile(prsettings))
        prsettings.setValue("m32/exteeprom", eeprom);
    return 0;
}

int CRP6Simulator::luaGetExtEEPROM(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);

    const int adr = luaL_checkint(l, 1);

    QVariantList eeprom(instance->getExtEEPROM());

    if (eeprom.isEmpty())
        lua_pushinteger(l, 255); // Something went wrong (255 is def. value)
    else
        lua_pushinteger(l, eeprom[adr].toUInt());

    return 1;
}

int CRP6Simulator::luaSetBeeperEnabled(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);

    const bool e = NLua::checkBoolean(l, 1);
    const bool playing = (SDL_GetAudioStatus() == SDL_AUDIO_PLAYING);

    if (e != playing)
    {
        instance->playingBeep = e;

        if (e)
        {
            instance->audioCurrentCycle = 0.0;
            instance->emit beeperPitchChanged(instance->beeperPitch);
        }
        else
            instance->emit beeperPitchChanged(0);

        // UNDONE: Check if audio is enabled/not in error state
        SDL_PauseAudio(!e);
    }

    return 0;
}

int CRP6Simulator::luaSetBeeperFrequency(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);

    instance->beeperPitch = luaL_checkint(l, 1);
    const float freq = luaL_checknumber(l, 2);

    // UNDONE: Check if audio is enabled/not in error state
    SDL_LockAudio();
    instance->audioFrequency = freq;
    SDL_UnlockAudio();

    if (instance->playingBeep)
        instance->emit beeperPitchChanged(instance->beeperPitch);

    return 0;
}

int CRP6Simulator::luaSetSoundCallback(lua_State *l)
{
    luaL_checktype(l, 1, LUA_TFUNCTION);
    if (instance->luaHandleSoundCallback)
        luaL_unref(l, LUA_REGISTRYINDEX, instance->luaHandleSoundCallback);
    lua_pushvalue(l, 1);
    instance->luaHandleSoundCallback = luaL_ref(l, LUA_REGISTRYINDEX);
    emit instance->receivedSoundCallback(true);
    return 0;
}

int CRP6Simulator::luaSetKeyPressCallback(lua_State *l)
{
    luaL_checktype(l, 1, LUA_TFUNCTION);
    if (instance->luaHandleKeyPressCallback)
        luaL_unref(l, LUA_REGISTRYINDEX, instance->luaHandleKeyPressCallback);
    lua_pushvalue(l, 1);
    instance->luaHandleKeyPressCallback = luaL_ref(l, LUA_REGISTRYINDEX);
    emit instance->receivedKeyPressCallback(true);
    return 0;
}

void CRP6Simulator::handleRobotSerialDeviceData()
{
    if (robotSerialSendLuaCallback == 0)
        return;

    // UNDONE: Ignore data if not running(? callback == 0 in that case)

    QByteArray txt = handleSerialDeviceData(robotSerialDevice,
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

    QByteArray txt = handleSerialDeviceData(m32SerialDevice,
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
    if (!checkMapChange())
        return;

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

bool CRP6Simulator::saveMap()
{
    Q_ASSERT(!currentMapFile.isEmpty());

    if (currentMapIsTemplate)
    {
        // If map is saved under another name it will be cleared
        // automatically from being a template.
        return saveMapAs();
    }

    QSettings s(currentMapFile, QSettings::IniFormat);

    if (!verifySettingsFile(s))
        return false;

    robotScene->saveMap(s);
    return true;
}

bool CRP6Simulator::saveMapAs()
{
    Q_ASSERT(!currentMapFile.isEmpty());

    const QString projdir = QFileInfo(currentProjectFile).path();
    const QString destfile =
            QFileDialog::getSaveFileName(this, "Save map as", projdir,
                                         "map files (*.map)");
    if (destfile.isEmpty())
        return false;

    QSettings s(destfile, QSettings::IniFormat);

    if (!verifySettingsFile(s))
        return false;

    robotScene->saveMap(s);

    // Sets up correct file selector settings etc.
    loadMapFile(destfile, false);

    return true;
}

void CRP6Simulator::loadMap()
{
    if (!checkMapChange())
        return;

    QString file =
            QFileDialog::getOpenFileName(this, "Open map file",
                                         QDir::homePath(),
                                         "RP6 map files (*.map)");
    if (!file.isEmpty())
        loadMapFile(file, false);
}

void CRP6Simulator::editProjectSettings()
{
    CProjectSettings prsettings(instance->currentProjectFile);
    if (!verifySettingsFile(prsettings))
        return;

    CProjectSettingsDialog dialog;
    dialog.loadSettings(prsettings);

    if (dialog.exec() == QDialog::Accepted)
        dialog.saveSettings(prsettings);
}

void CRP6Simulator::showAbout()
{
    // UNDONE
    QMessageBox::about(this, "rp6sim", "Hello");
}

void CRP6Simulator::timedUIUpdate()
{
    QVariantHash adcvalues(getADCValues(robotSimulator->getLuaState()));
    for (QVariantHash::iterator it=adcvalues.begin(); it!=adcvalues.end(); ++it)
    {
        appendRobotStatusUpdate(QStringList() << "robot" << "ADC" <<
                                it.key() << it.value().toString());
        robotWidget->setRobotADCValue(it.key(), it.value().toInt());
    }

    adcvalues = getADCValues(m32Simulator->getLuaState());
    for (QVariantHash::iterator it=adcvalues.begin(); it!=adcvalues.end(); ++it)
    {
        appendRobotStatusUpdate(QStringList() << "m32" << "ADC" <<
                                it.key() << it.value().toString());
        robotWidget->setM32ADCValue(it.key(), it.value().toInt());
    }


#ifdef USEATOMIC
    const QAtomicInt *ioregisters;
#else
    QReadLocker iolocker(&simulator->getIORegisterLock());
    const TIORegisterData *ioregisters;
#endif

    if (IORegisterTableSelector->currentText() == "m32")
        ioregisters = m32Simulator->getIORegisterArray();
    else
        ioregisters = robotSimulator->getIORegisterArray();

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
    statlocker.unlock(); // So we can call appendRobotStatusUpdate() below

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
    robotScene->start();
    robotWidget->start();

    runPluginAction->setEnabled(false);
    stopPluginAction->setEnabled(true);
    resetPluginAction->setEnabled(true);
    ADCDockWidget->setEnabled(true);
}

void CRP6Simulator::stopPlugin()
{
    SDL_PauseAudio(1);

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

    if (m32SerialSendLuaCallback || luaHandleExtInt1Callback ||
        luaHandleSoundCallback || luaHandleKeyPressCallback)
    {
        NLua::CLuaLocker lualocker(m32Simulator->getLuaState());

        if (m32SerialSendLuaCallback)
        {
            luaL_unref(m32Simulator->getLuaState(), LUA_REGISTRYINDEX,
                       m32SerialSendLuaCallback);
            m32SerialSendLuaCallback = 0;
        }

        if (luaHandleExtInt1Callback)
        {
            luaL_unref(m32Simulator->getLuaState(), LUA_REGISTRYINDEX,
                       luaHandleExtInt1Callback);
            luaHandleExtInt1Callback = 0;
        }

        if (luaHandleSoundCallback)
        {
            luaL_unref(m32Simulator->getLuaState(), LUA_REGISTRYINDEX,
                       luaHandleSoundCallback);
            luaHandleSoundCallback = 0;
        }

        if (luaHandleKeyPressCallback)
        {
            luaL_unref(m32Simulator->getLuaState(), LUA_REGISTRYINDEX,
                       luaHandleKeyPressCallback);
            luaHandleKeyPressCallback = 0;
        }
    }

    robotSimulator->stopPlugin();
    m32Simulator->stopPlugin();

    pluginUpdateUITimer->stop();
    pluginUpdateLEDsTimer->stop();
    robotScene->stop();
    robotWidget->stop();

    // Dump any remaining buffered data
    timedUIUpdate();
    timedLEDUpdate();

    runPluginAction->setEnabled(true);
    stopPluginAction->setEnabled(false);
    resetPluginAction->setEnabled(false);
    ADCDockWidget->setEnabled(false);

    robotSerialSendButton->setEnabled(false);
    IRCOMMSendButton->setEnabled(false);
    handClapButton->setEnabled(false);
    keyPadWidget->setEnabled(false);

    robotIsBlocked = false;
}

void CRP6Simulator::doHandClap()
{
    if (!playingHandClap)
    {
        if (luaHandleSoundCallback)
        {
            NLua::CLuaLocker lualock(m32Simulator->getLuaState());
            lua_rawgeti(m32Simulator->getLuaState(), LUA_REGISTRYINDEX,
                        luaHandleSoundCallback);
            switch (handClapMode)
            {
            case CLAP_SOFT:
                lua_pushstring(m32Simulator->getLuaState(), "soft");
                break;
            case CLAP_NORMAL:
                lua_pushstring(m32Simulator->getLuaState(), "normal");
                break;
            case CLAP_LOUD:
                lua_pushstring(m32Simulator->getLuaState(), "loud");
                break;
            }
            lua_call(m32Simulator->getLuaState(), 1, 0);
        }

        if (handClapCVT.buf)
        {
            handClapSoundPos = 0;
            playingHandClap = true;
            SDL_PauseAudio(0);
        }
    }
}

void CRP6Simulator::setHandClapMode(int m)
{
    handClapMode = static_cast<EHandClapMode>(m);
    qDebug() << "Hand clap:" << m;
    switch (handClapMode)
    {
    case CLAP_SOFT: handClapButton->setText("soft"); break;
    case CLAP_NORMAL: handClapButton->setText("normal"); break;
    case CLAP_LOUD: handClapButton->setText("loud"); break;
    }
}

void CRP6Simulator::handleKeyPadPress(int key)
{
    qDebug() << "Key press:" << key;
    if (luaHandleKeyPressCallback)
    {
        NLua::CLuaLocker lualock(m32Simulator->getLuaState());
        lua_rawgeti(m32Simulator->getLuaState(), LUA_REGISTRYINDEX,
                    luaHandleKeyPressCallback);
        lua_pushnumber(m32Simulator->getLuaState(), key);
        lua_call(m32Simulator->getLuaState(), 1, 0);
    }
}

void CRP6Simulator::resetKeyPress()
{
    qDebug() << "Reset key press";
    if (luaHandleKeyPressCallback)
    {
        NLua::CLuaLocker lualock(m32Simulator->getLuaState());
        lua_rawgeti(m32Simulator->getLuaState(), LUA_REGISTRYINDEX,
                    luaHandleKeyPressCallback);
        lua_pushnil(m32Simulator->getLuaState());
        lua_call(m32Simulator->getLuaState(), 1, 0);
    }
}

void CRP6Simulator::handleDataPlotSelection(QAction *a)
{
    CRobotWidget::EDataPlotType plot =
            static_cast<CRobotWidget::EDataPlotType>(a->data().toInt());
    robotWidget->showDataPlot(plot);
    a->setVisible(false);
}

void CRP6Simulator::enableDataPlotSelection(int plot)
{
    QList<QAction *> actions = dataPlotMenu->actions();
    foreach (QAction *a, actions)
    {
        if (a->data() == plot)
        {
            a->setVisible(true);
            break;
        }
    }
}

void CRP6Simulator::tabViewChanged(int index)
{
    // 0: Robot tab; 1: Map tab
    const bool robottab = (index == 0);

    robotToolBar->setEnabled(robottab);

    mapSelectorTreeWidget->setEnabled(!robottab);
    editMapToolBar->setEnabled(!robottab && !currentMapFile.isEmpty());
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
    dialog.setAmbientLight(robotScene->getAmbientLight());
    dialog.setShadowQuality(robotScene->getShadowQuality());
    dialog.setGridSize(robotScene->getGridSize());
    dialog.setAutoGridSnap(robotScene->getAutoGrid());

    if (dialog.exec() == QDialog::Accepted)
    {
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

    if (!checkMapChange())
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

    const int rsize = robotADCTableWidget->rowCount();
    const int msize = m32ADCTableWidget->rowCount();
    const int maxsize = qMax(rsize, msize);

    for (int i=0; i<maxsize; ++i)
    {
        if (i < rsize)
            getADCTableCheckBox(robotADCTableWidget, i, 0)->setChecked(false);
        if (i < msize)
            getADCTableCheckBox(m32ADCTableWidget, i, 0)->setChecked(false);
    }

    applyADCTable();
}

void CRP6Simulator::applyADCTable()
{
    ::applyADCTable(robotSimulator->getLuaState(), robotADCTableWidget);
    ::applyADCTable(m32Simulator->getLuaState(), m32ADCTableWidget);
}

void CRP6Simulator::updateExtEEPROM(int page)
{
    const QVariantList eeprom(getExtEEPROM());
    const int startindex = EXTEEPROM_PAGESIZE * page;

    int index = startindex;
    for (int i=0; i<EXTEEPROM_TABLEDIMENSION; ++i)
    {
        for (int j=0; j<EXTEEPROM_TABLEDIMENSION; ++j)
        {
            if (extEEPROMMode == EXTEEPROMMODE_NUMERICAL)
            {
                const int val = eeprom[index].toUInt();
                extEEPROMTableWidget->item(i, j)->setText(QString::number(val));
            }
            else
            {
                const char val[] =  { eeprom[index].toUInt(), 0 };
                extEEPROMTableWidget->item(i, j)->setText(val);
            }
            ++index;
        }
    }

    if (extEEPROMTableWidget->currentItem())
        updateExtEEPROMSelection(extEEPROMTableWidget->currentItem());
}

void CRP6Simulator::updateExtEEPROMMode(int mode)
{
    extEEPROMMode = (mode == 1) ? EXTEEPROMMODE_CHAR : EXTEEPROMMODE_NUMERICAL;
    extEEPROMInputStackedWidget->setCurrentIndex(extEEPROMMode);
    updateExtEEPROM(extEEPROMPageSpinBox->value());
}

void CRP6Simulator::updateExtEEPROMSelection(QTableWidgetItem *item)
{
    if (extEEPROMMode == EXTEEPROMMODE_NUMERICAL)
        extEEPROMInputSpinBox->setValue(item->text().toUInt());
    else
        extEEPROMInputLineEdit->setText(item->text());
}

void CRP6Simulator::updateExtEEPROMByte(int address, quint8 data)
{
    const int page = address / EXTEEPROM_PAGESIZE;

    if (page == extEEPROMPageSpinBox->value())
    {
        const int reladr = address % EXTEEPROM_PAGESIZE;
        const int row = reladr / EXTEEPROM_TABLEDIMENSION;
        const int col = reladr % EXTEEPROM_TABLEDIMENSION;
        qDebug() << "updateEEPROMByte:" << page << reladr << row << col;
        QTableWidgetItem *item = extEEPROMTableWidget->item(row, col);
        if (extEEPROMMode == EXTEEPROMMODE_NUMERICAL)
            item->setText(QString::number(data));
        else
            item->setText(QChar(data));
    }
}

void CRP6Simulator::applyExtEEPROMChange()
{
    Q_ASSERT(extEEPROMTableWidget->currentItem());

    int adr = extEEPROMPageSpinBox->value() * EXTEEPROM_PAGESIZE;
    adr += (extEEPROMTableWidget->currentItem()->row() * EXTEEPROM_TABLEDIMENSION);
    adr += extEEPROMTableWidget->currentItem()->column();

    uint8_t data;
    if (extEEPROMMode == EXTEEPROMMODE_NUMERICAL)
        data = extEEPROMInputSpinBox->value();
    else
        data = extEEPROMInputLineEdit->text().toAscii()[0];

    QVariantList eeprom(getExtEEPROM());

    if (eeprom.isEmpty())
        return; // Something went wrong

    eeprom[adr] = data;
    extEEPROMByteChanged(adr, data);

    CProjectSettings prsettings(instance->currentProjectFile);
    if (verifySettingsFile(prsettings))
        prsettings.setValue("m32/exteeprom", eeprom);
}

void CRP6Simulator::setLuaBumper(CBumper *b, bool e)
{
    NLua::CLuaLocker lualocker(robotSimulator->getLuaState());
    lua_rawgeti(robotSimulator->getLuaState(), LUA_REGISTRYINDEX,
                bumperLuaCallbacks[b]);
    lua_pushboolean(robotSimulator->getLuaState(), e);
    lua_call(robotSimulator->getLuaState(), 1, 0);
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
    // NOTE: Currently the lua state of the robot simulator is used for both
    //       robot AND m32 drivers.
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
