#include "rp6simul.h"
#include "pluginthread.h"
#include "avrtimer.h"
#include "lua.h"
#include "projectwizard.h"
#include "projectsettings.h"

#include <QtGui>
#include <QLibrary>

namespace {

// Keep in sync with EIORegisterTypes
const char *IORegisterStringArray[IO_END] = {
    // UART
    "UCSRA",
    "UCSRB",
    "UCSRC",
    "UBRR",
    "UBRRL",
    "UBRRH",
    "UDR",

    // Timer0
    "TCCR0",
    "OCR0",

    // Timer 1
    "TCCR1A",
    "TCCR1B",
    "OCR1A",
    "OCR1B",
    "OCR1AL",
    "OCR1AH",
    "OCR1BL",
    "OCR1BH",
    "ICR1",

    // Timer2
    "TCCR2",
    "OCR2",

    // TIMSK
    "TIMSK",

    // General IO
    "PORTA",
    "PORTB",
    "PORTC",
    "PORTD",
    "DDRA",
    "DDRB",
    "DDRC",
    "DDRD",
    "PINA",
    "PINB",
    "PINC",
    "PIND",

    // ADC
    "ADMUX",
    "ADCSRA",
    "SFIOR",
    "ADC",

    // External interrupts
    "MCUCR",
    "GICR",
    "MCUCSR",
};


template <typename TF> TF getLibFunc(QLibrary &lib, const char *f)
{
    TF ret = (TF)lib.resolve(f);

    if (!ret)
        qDebug() << "Failed to get lib function" << f;

    return ret;
}

QString constructISRFunc(EISRTypes type)
{
    QString f;

    switch (type)
    {
    case ISR_USART_RXC_vect: f = "USART_RXC_vect"; break;
    case ISR_TIMER0_COMP_vect: f = "TIMER0_COMP_vect"; break;
    case ISR_TIMER1_COMPA_vect: f = "TIMER1_COMPA_vect"; break;
    case ISR_TIMER2_COMP_vect: f = "TIMER2_COMP_vect"; break;
    default: Q_ASSERT(false); break;
    }

    return "ISR_" + f;
}

const char *getCString(const QString &s)
{
    static QByteArray by;
    by = s.toLatin1();
    return by.data();
}

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

}

CRP6Simulator *CRP6Simulator::instance = 0;

CRP6Simulator::CRP6Simulator(QWidget *parent) : QMainWindow(parent),
    pluginMainThread(0)
{
    Q_ASSERT(!instance);
    instance = this;

    resize(850, 600);

    projectWizard = new CProjectWizard(this);

    createMenus();
    createToolbars();

    QSplitter *splitter = new QSplitter(Qt::Vertical, this);
    setCentralWidget(splitter);

    splitter->addWidget(createMainWidget());
    splitter->addWidget(createLogWidgets());
    splitter->setSizes(QList<int>() << 600 << 300);

    QDockWidget *statdock, *regdock;
    addDockWidget(Qt::RightDockWidgetArea, statdock = createStatusDock(),
                  Qt::Vertical);
    addDockWidget(Qt::RightDockWidgetArea, regdock = createRegisterDock(),
                  Qt::Vertical);
    tabifyDockWidget(statdock, regdock);
    activateDockTab(this, "Status");

    pluginUpdateUITimer = new QTimer(this);
    connect(pluginUpdateUITimer, SIGNAL(timeout()), this, SLOT(timedUpdate()));
    pluginUpdateUITimer->setInterval(250);

    initAVRClock();
    initLua();
}

CRP6Simulator::~CRP6Simulator()
{
    terminateAVRClock();
    delete AVRClock;
    terminatePluginMainThread();
}

void CRP6Simulator::createMenus()
{
    QMenu *menu = menuBar()->addMenu("&File");
    menu->addAction("New", this, SLOT(newProject()), tr("ctrl+N"));
    menu->addAction("Open", this, SLOT(openProject()), tr("ctrl+O"));
    menu->addMenu("Open recent...")->addAction("blah");
    menu->addSeparator();
    menu->addAction("Quit", this, SLOT(close()), tr("ctrl+Q"));

    menu = menuBar()->addMenu("&Edit");
    menu->addAction("Project settings");
    menu->addSeparator();
    menu->addAction("Preferences");

    menu = menuBar()->addMenu("&Help");
    menu->addAction("About");
    menu->addAction("About Qt", qApp, SLOT(aboutQt()));
}

void CRP6Simulator::createToolbars()
{
    QToolBar *toolb = addToolBar("Run");
    toolb->addAction(QIcon(style()->standardIcon(QStyle::SP_ArrowRight)), "Run",
                     this, SLOT(runPlugin()));
}

QWidget *CRP6Simulator::createMainWidget()
{
    QWidget *ret = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout(ret);

    QPushButton *button = new QPushButton("Start");
    connect(button, SIGNAL(clicked()), this, SLOT(runPlugin()));
    vbox->addWidget(button);

    return ret;
}

QWidget *CRP6Simulator::createLogWidgets()
{
    QTabWidget *ret = new QTabWidget;
    ret->setTabPosition(QTabWidget::South);

    ret->addTab(logWidget = new QPlainTextEdit, "log");
    logWidget->setReadOnly(true);
    logWidget->setCenterOnScroll(true);
    connect(this, SIGNAL(logTextReady(const QString &)), logWidget,
            SLOT(appendHtml(const QString &)));

    ret->addTab(serialOutputWidget = new QPlainTextEdit, "serial");
    serialOutputWidget->setReadOnly(true);
    serialOutputWidget->setCenterOnScroll(true);

    return ret;
}

QDockWidget *CRP6Simulator::createStatusDock()
{
    QDockWidget *ret = new QDockWidget("Status", this);
    ret->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QWidget *w = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout(w);
    ret->setWidget(w);

    QGroupBox *group = new QGroupBox("Clock");
    vbox->addWidget(group);
    QFormLayout *form = new QFormLayout(group);

    form->addRow("RP6 clock (MHz)", clockDisplay = new QLCDNumber(4));
    form->addRow("m32 clock (MHz)", new QLCDNumber(4)); // UNDONE

    group = new QGroupBox("Robot");
    vbox->addWidget(group);
    QVBoxLayout *subvbox = new QVBoxLayout(group);

    QTreeWidget *tree = new QTreeWidget;
    tree->setHeaderLabels(QStringList() << "Device" << "Value");
    QTreeWidgetItem *item = new QTreeWidgetItem(tree, QStringList() << "Motor");
    QList<QTreeWidgetItem *> children;
    children << new QTreeWidgetItem(QStringList() << "Power") <<
                new QTreeWidgetItem(QStringList() << "Direction") <<
                new QTreeWidgetItem(QStringList() << "Speed") <<
                new QTreeWidgetItem(QStringList() << "Current");
    item->addChildren(children);
    subvbox->addWidget(tree);

    group = new QGroupBox("Drivers");
    vbox->addWidget(group);
    subvbox = new QVBoxLayout(group);

    tree = new QTreeWidget;
    tree->setHeaderLabels(QStringList() << "Driver" << "Status");
    tree->setRootIsDecorated(false);
    QList<QTreeWidgetItem *> items;
    items << new QTreeWidgetItem(QStringList() << "leds") <<
             new QTreeWidgetItem(QStringList() << "timer0") <<
             new QTreeWidgetItem(QStringList() << "timer1") <<
             new QTreeWidgetItem(QStringList() << "timer2") <<
             new QTreeWidgetItem(QStringList() << "motor");
    tree->addTopLevelItems(items);
    subvbox->addWidget(tree);

    return ret;
}

QDockWidget *CRP6Simulator::createRegisterDock()
{
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

void CRP6Simulator::initAVRClock()
{
    // From http://labs.qt.nokia.com/2010/06/17/youre-doing-it-wrong/

    AVRClock = new CAVRClock;
    connect(AVRClock, SIGNAL(clockSpeed(unsigned long)), this,
            SLOT(updateClockDisplay(unsigned long)));

    AVRClockThread = new QThread(this);
    connect(AVRClockThread, SIGNAL(finished()), AVRClock, SLOT(stop()));
    AVRClock->moveToThread(AVRClockThread);
    AVRClockThread->start();
}

void CRP6Simulator::setLuaIOTypes()
{
    for (int i=0; i<IO_END; ++i)
    {
        QString io = QString("IO_%1").arg(IORegisterStringArray[i]);
        NLua::setVariable(i, getCString(io), "avr");
    }
}

void CRP6Simulator::setLuaAVRConstants()
{
    // Evil macros and stuff
#define SET_LUA_CONSTANT(C) NLua::setVariable(C, #C, "avr")

    // Timer0
    SET_LUA_CONSTANT(WGM00);
    SET_LUA_CONSTANT(COM01);
    SET_LUA_CONSTANT(COM00);
    SET_LUA_CONSTANT(WGM01);
    SET_LUA_CONSTANT(CS02);
    SET_LUA_CONSTANT(CS01);
    SET_LUA_CONSTANT(CS00);

    // Timer1A
    SET_LUA_CONSTANT(COM1A1);
    SET_LUA_CONSTANT(COM1A0);
    SET_LUA_CONSTANT(COM1B1);
    SET_LUA_CONSTANT(COM1B0);
    SET_LUA_CONSTANT(WGM11);
    SET_LUA_CONSTANT(WGM10);

    // Timer1B
    SET_LUA_CONSTANT(WGM13);
    SET_LUA_CONSTANT(WGM12);
    SET_LUA_CONSTANT(CS12);
    SET_LUA_CONSTANT(CS11);
    SET_LUA_CONSTANT(CS10);

    // Timer2
    SET_LUA_CONSTANT(WGM20);
    SET_LUA_CONSTANT(COM21);
    SET_LUA_CONSTANT(COM20);
    SET_LUA_CONSTANT(WGM21);
    SET_LUA_CONSTANT(CS22);
    SET_LUA_CONSTANT(CS21);
    SET_LUA_CONSTANT(CS20);

    // TIMSK
    SET_LUA_CONSTANT(OCIE2);
    SET_LUA_CONSTANT(OCIE1A);
    SET_LUA_CONSTANT(OCIE1B);
    SET_LUA_CONSTANT(OCIE0);

    // UART
    SET_LUA_CONSTANT(RXC);
    SET_LUA_CONSTANT(TXC);
    SET_LUA_CONSTANT(UDRE);
    SET_LUA_CONSTANT(FE);
    SET_LUA_CONSTANT(DOR);
    SET_LUA_CONSTANT(PE);
    SET_LUA_CONSTANT(U2X);
    SET_LUA_CONSTANT(MPCM);
    SET_LUA_CONSTANT(RXCIE);
    SET_LUA_CONSTANT(TXCIE);
    SET_LUA_CONSTANT(UDRIE);
    SET_LUA_CONSTANT(RXEN);
    SET_LUA_CONSTANT(TXEN);
    SET_LUA_CONSTANT(RXB8);
    SET_LUA_CONSTANT(TXB8);
    SET_LUA_CONSTANT(URSEL);
    SET_LUA_CONSTANT(UMSEL);
    SET_LUA_CONSTANT(UPM1);
    SET_LUA_CONSTANT(UPM0);
    SET_LUA_CONSTANT(USBS);
    SET_LUA_CONSTANT(UCSZ1);
    SET_LUA_CONSTANT(UCSZ0);
    SET_LUA_CONSTANT(UCPOL);

    // General IO - Port A
    SET_LUA_CONSTANT(PA7);
    SET_LUA_CONSTANT(PA6);
    SET_LUA_CONSTANT(PA5);
    SET_LUA_CONSTANT(PA4);
    SET_LUA_CONSTANT(PA3);
    SET_LUA_CONSTANT(PA2);
    SET_LUA_CONSTANT(PA1);
    SET_LUA_CONSTANT(PA0);
    SET_LUA_CONSTANT(DDA7);
    SET_LUA_CONSTANT(DDA6);
    SET_LUA_CONSTANT(DDA5);
    SET_LUA_CONSTANT(DDA4);
    SET_LUA_CONSTANT(DDA3);
    SET_LUA_CONSTANT(DDA2);
    SET_LUA_CONSTANT(DDA1);
    SET_LUA_CONSTANT(DDA0);
    SET_LUA_CONSTANT(PINA7);
    SET_LUA_CONSTANT(PINA6);
    SET_LUA_CONSTANT(PINA5);
    SET_LUA_CONSTANT(PINA4);
    SET_LUA_CONSTANT(PINA3);
    SET_LUA_CONSTANT(PINA2);
    SET_LUA_CONSTANT(PINA1);
    SET_LUA_CONSTANT(PINA0);

    // General IO - Port B
    SET_LUA_CONSTANT(PB7);
    SET_LUA_CONSTANT(PB6);
    SET_LUA_CONSTANT(PB5);
    SET_LUA_CONSTANT(PB4);
    SET_LUA_CONSTANT(PB3);
    SET_LUA_CONSTANT(PB2);
    SET_LUA_CONSTANT(PB1);
    SET_LUA_CONSTANT(PB0);
    SET_LUA_CONSTANT(DDB7);
    SET_LUA_CONSTANT(DDB6);
    SET_LUA_CONSTANT(DDB5);
    SET_LUA_CONSTANT(DDB4);
    SET_LUA_CONSTANT(DDB3);
    SET_LUA_CONSTANT(DDB2);
    SET_LUA_CONSTANT(DDB1);
    SET_LUA_CONSTANT(DDB0);
    SET_LUA_CONSTANT(PINB7);
    SET_LUA_CONSTANT(PINB6);
    SET_LUA_CONSTANT(PINB5);
    SET_LUA_CONSTANT(PINB4);
    SET_LUA_CONSTANT(PINB3);
    SET_LUA_CONSTANT(PINB2);
    SET_LUA_CONSTANT(PINB1);
    SET_LUA_CONSTANT(PINB0);

    // General IO - Port C
    SET_LUA_CONSTANT(PC7);
    SET_LUA_CONSTANT(PC6);
    SET_LUA_CONSTANT(PC5);
    SET_LUA_CONSTANT(PC4);
    SET_LUA_CONSTANT(PC3);
    SET_LUA_CONSTANT(PC2);
    SET_LUA_CONSTANT(PC1);
    SET_LUA_CONSTANT(PC0);
    SET_LUA_CONSTANT(DDC7);
    SET_LUA_CONSTANT(DDC6);
    SET_LUA_CONSTANT(DDC5);
    SET_LUA_CONSTANT(DDC4);
    SET_LUA_CONSTANT(DDC3);
    SET_LUA_CONSTANT(DDC2);
    SET_LUA_CONSTANT(DDC1);
    SET_LUA_CONSTANT(DDC0);
    SET_LUA_CONSTANT(PINC7);
    SET_LUA_CONSTANT(PINC6);
    SET_LUA_CONSTANT(PINC5);
    SET_LUA_CONSTANT(PINC4);
    SET_LUA_CONSTANT(PINC3);
    SET_LUA_CONSTANT(PINC2);
    SET_LUA_CONSTANT(PINC1);
    SET_LUA_CONSTANT(PINC0);

    // General IO - Port D
    SET_LUA_CONSTANT(PD7);
    SET_LUA_CONSTANT(PD6);
    SET_LUA_CONSTANT(PD5);
    SET_LUA_CONSTANT(PD4);
    SET_LUA_CONSTANT(PD3);
    SET_LUA_CONSTANT(PD2);
    SET_LUA_CONSTANT(PD1);
    SET_LUA_CONSTANT(PD0);
    SET_LUA_CONSTANT(DDD7);
    SET_LUA_CONSTANT(DDD6);
    SET_LUA_CONSTANT(DDD5);
    SET_LUA_CONSTANT(DDD4);
    SET_LUA_CONSTANT(DDD3);
    SET_LUA_CONSTANT(DDD2);
    SET_LUA_CONSTANT(DDD1);
    SET_LUA_CONSTANT(DDD0);
    SET_LUA_CONSTANT(PIND7);
    SET_LUA_CONSTANT(PIND6);
    SET_LUA_CONSTANT(PIND5);
    SET_LUA_CONSTANT(PIND4);
    SET_LUA_CONSTANT(PIND3);
    SET_LUA_CONSTANT(PIND2);
    SET_LUA_CONSTANT(PIND1);
    SET_LUA_CONSTANT(PIND0);

    // ADC
    SET_LUA_CONSTANT(ADEN);
    SET_LUA_CONSTANT(ADSC);
    SET_LUA_CONSTANT(ADATE);
    SET_LUA_CONSTANT(ADIF);
    SET_LUA_CONSTANT(ADIE);
    SET_LUA_CONSTANT(ADPS2);
    SET_LUA_CONSTANT(ADPS1);
    SET_LUA_CONSTANT(ADPS0);
    SET_LUA_CONSTANT(REFS1);
    SET_LUA_CONSTANT(REFS0);
    SET_LUA_CONSTANT(ADLAR);
    SET_LUA_CONSTANT(MUX4);
    SET_LUA_CONSTANT(MUX3);
    SET_LUA_CONSTANT(MUX2);
    SET_LUA_CONSTANT(MUX1);
    SET_LUA_CONSTANT(MUX0);

    // External interrupts
    SET_LUA_CONSTANT(INT1);
    SET_LUA_CONSTANT(INT0);
    SET_LUA_CONSTANT(INT2);
    SET_LUA_CONSTANT(IVSEL);
    SET_LUA_CONSTANT(IVCE);
    SET_LUA_CONSTANT(SE);
    SET_LUA_CONSTANT(SM2);
    SET_LUA_CONSTANT(SM1);
    SET_LUA_CONSTANT(SM0);
    SET_LUA_CONSTANT(ISC11);
    SET_LUA_CONSTANT(ISC10);
    SET_LUA_CONSTANT(ISC01);
    SET_LUA_CONSTANT(ISC00);
    SET_LUA_CONSTANT(JTD);
    SET_LUA_CONSTANT(ISC2);
    SET_LUA_CONSTANT(JTRF);
    SET_LUA_CONSTANT(WDRF);
    SET_LUA_CONSTANT(BORF);
    SET_LUA_CONSTANT(EXTRF);
    SET_LUA_CONSTANT(PORF);

    // ISRs
    SET_LUA_CONSTANT(ISR_USART_RXC_vect);
    SET_LUA_CONSTANT(ISR_TIMER0_COMP_vect);
    SET_LUA_CONSTANT(ISR_TIMER1_COMPA_vect);
    SET_LUA_CONSTANT(ISR_TIMER2_COMP_vect);

#undef SET_LUA_CONSTANT
}

void CRP6Simulator::initLua()
{
    setLuaIOTypes();
    setLuaAVRConstants();

    // avr
    NLua::registerFunction(luaAvrGetIORegister, "getIORegister", "avr");
    NLua::registerFunction(luaAvrSetIORegister, "setIORegister", "avr");

    // clock
    NLua::registerFunction(luaClockCreateTimer, "createTimer", "clock");
    NLua::registerFunction(luaClockEnableTimer, "enableTimer", "clock");

    // timer class
    NLua::registerClassFunction(luaTimerSetCompareValue, "setCompareValue", "timer");
    NLua::registerClassFunction(luaTimerSetPrescaler, "setPrescaler", "timer");
    NLua::registerClassFunction(luaTimerSetTimeOut, "setTimeOut", "timer");
    NLua::registerClassFunction(luaTimerIsEnabled, "isEnabled", "timer");

    // bit
    NLua::registerFunction(luaBitIsSet, "isSet", "bit");
    NLua::registerFunction(luaBitSet, "set", "bit");
    NLua::registerFunction(luaBitUnSet, "unSet", "bit");
    NLua::registerFunction(luaBitUnPack, "unPack", "bit");
    NLua::registerFunction(luaBitAnd, "bitAnd", "bit");

    // global
    NLua::registerFunction(luaAppendLogOutput, "appendLogOutput");
    NLua::registerFunction(luaAppendSerialOutput, "appendSerialOutput");

    NLua::luaInterface.exec();

    // UNDONE: Needed?
    lua_getglobal(NLua::luaInterface, "init");
    lua_call(NLua::luaInterface, 0, 0);
}

void CRP6Simulator::terminateAVRClock()
{
    qDebug() << "Terminating AVR clock thread";
    AVRClockThread->quit();
    AVRClockThread->wait();
}

void CRP6Simulator::terminatePluginMainThread()
{
    if (pluginMainThread)
    {
        qDebug() << "Terminating plugin main thread";
        pluginMainThread->terminate();

        // UNDONE: Use terminated slot
        // UNDONE: Thread does not always exit
        if (!pluginMainThread->wait(250))
            qDebug() << "Failed to terminate plugin main thread!";

        delete pluginMainThread;
        pluginMainThread = 0;
    }
}

void CRP6Simulator::openProjectFile(const QString &file)
{
    CProjectSettings prsettings(file);
    prsettings.sync();

    if (prsettings.status() == QSettings::AccessError)
        QMessageBox::critical(this, "File access error", "Unable to access project file!");
    else if (prsettings.status() == QSettings::FormatError)
        QMessageBox::critical(this, "File format error", "Invalid file format!");
    else
    {
        closeProject();
        initPlugin();
    }
}

void CRP6Simulator::closeProject()
{
    lua_getglobal(NLua::luaInterface, "closePlugin");
    lua_call(NLua::luaInterface, 0, 0);

    AVRClock->stop();
    AVRClock->reset();

    terminatePluginMainThread();

    for (int i=0; i<IO_END; ++i)
        IORegisterData[i] = 0;

    ISRsEnabled = false; // UNDONE: ISRs are enabled by default?

    for (int i=0; i<ISR_END; ++i)
    {
        ISRCacheArray[i] = 0;
        ISRFailedArray[i] = false;
    }
}

void CRP6Simulator::initPlugin()
{
    // UNDONE: unload previous library (? probably only name is cached)

    QLibrary lib("../test/build/libmyrp6.so"); // UNDONE

    if (!lib.load())
    {
        QMessageBox::critical(this, "Error",
                              "Failed to load library:\n" + lib.errorString());
        return;
    }

    TCallPluginMainFunc mainfunc =
            getLibFunc<TCallPluginMainFunc>(lib, "callAvrMain");
    TSetPluginCallbacks setplugincb =
            getLibFunc<TSetPluginCallbacks>(lib, "setPluginCallbacks");

    if (!mainfunc || !setplugincb)
        return; // UNDONE

    AVRClock->stop();
    AVRClock->reset();

    terminatePluginMainThread();

    pluginMainThread = new CCallPluginMainThread(mainfunc, this);

    setplugincb(IORegisterSetCB, IORegisterGetCB, enableISRsCB);

    for (int i=0; i<IO_END; ++i)
        IORegisterData[i] = 0;

    ISRsEnabled = false; // UNDONE: ISRs are enabled by default?

    for (int i=0; i<ISR_END; ++i)
    {
        ISRCacheArray[i] = 0;
        ISRFailedArray[i] = false;
    }

    lua_getglobal(NLua::luaInterface, "initPlugin");
    lua_call(NLua::luaInterface, 0, 0);

    pluginUpdateUITimer->start();
}

void CRP6Simulator::checkPluginThreadDelay()
{
    // This function is called whenever register data is accessed (get/set)
    // from the RP6 plugin thread. It adds a small delay (nanosleep) within small
    // intervals to prevent constant CPU hammering by the plugin. This
    // obviously will only work when the RP6 program accesses IO, however this
    // is commonly done.

    // Plugin thread running? (ISRs may also call this function which do not
    // have to be delayed)
    if (QThread::currentThread() == pluginMainThread)
    {
        // Init?
        if (!lastPluginDelay.tv_sec && !lastPluginDelay.tv_nsec)
            clock_gettime(CLOCK_MONOTONIC, &lastPluginDelay);
        else
        {
            timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &lastPluginDelay);
            const unsigned long delta = getUSDiff(lastPluginDelay, ts);

            if (delta > 50) // Delay every 50 us
            {
                lastPluginDelay = ts;
                ts.tv_sec = 0; ts.tv_nsec = 1000;
                nanosleep(&ts, 0);
            }
        }
    }
}

QString CRP6Simulator::getLogOutput(ELogType type, QString text) const
{
    // Html doesn't like tabs too much
    text = text.replace('\t', QString("&nbsp;").repeated(4));
    text = Qt::escape(text);
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

void CRP6Simulator::appendLogOutput(ELogType type, const QString &text)
{
    logWidget->appendHtml(getLogOutput(type, text));
}

void CRP6Simulator::IORegisterSetCB(EIORegisterTypes type, TIORegisterData data)
{
    instance->setIORegister(type, data);

    NLua::CLuaLocker lualocker;
    lua_getglobal(NLua::luaInterface, "handleIOData");
    lua_pushinteger(NLua::luaInterface, type);
    lua_pushinteger(NLua::luaInterface, data);

    lua_call(NLua::luaInterface, 2, 0);

    lualocker.unlock();

    instance->checkPluginThreadDelay();
}

TIORegisterData CRP6Simulator::IORegisterGetCB(EIORegisterTypes type)
{
    instance->checkPluginThreadDelay();
    return instance->getIORegister(type);
}

void CRP6Simulator::enableISRsCB(bool e)
{
    QMutexLocker lock(&instance->ISRExecMutex);
    instance->ISRsEnabled = e;
}

int CRP6Simulator::luaAvrGetIORegister(lua_State *l)
{
    NLua::CLuaLocker lualocker;
    const EIORegisterTypes type = static_cast<EIORegisterTypes>(luaL_checkint(l, 1));
    lua_pushinteger(l, instance->getIORegister(type));
    return 1;
}

int CRP6Simulator::luaAvrSetIORegister(lua_State *l)
{
    NLua::CLuaLocker lualocker;
    const EIORegisterTypes type = static_cast<EIORegisterTypes>(luaL_checkint(l, 1));
    const int data = luaL_checkint(l, 2);
    instance->setIORegister(type, data);
    return 0;
}

int CRP6Simulator::luaClockCreateTimer(lua_State *l)
{
    NLua::CLuaLocker lualocker;
    CAVRTimer *timer = instance->AVRClock->createTimer();
    NLua::createClass(l, timer, "timer", luaTimerDestr);
    return 1;
}

int CRP6Simulator::luaClockEnableTimer(lua_State *l)
{
    NLua::CLuaLocker lualocker;
    CAVRTimer *timer = NLua::checkClassData<CAVRTimer>(l, 1, "timer");
    const bool e = NLua::checkBoolean(l, 2);
    instance->AVRClock->enableTimer(timer, e);
    return 0;
}

int CRP6Simulator::luaTimerDestr(lua_State *l)
{
    qDebug() << "Removing timer";

    NLua::CLuaLocker lualocker;
    CAVRTimer *timer = NLua::checkClassData<CAVRTimer>(l, 1, "timer");
    instance->AVRClock->removeTimer(timer);
    delete timer;
    return 0;
}

int CRP6Simulator::luaTimerSetCompareValue(lua_State *l)
{
    NLua::CLuaLocker lualocker;
    CAVRTimer *timer = NLua::checkClassData<CAVRTimer>(l, 1, "timer");
    const int compare = luaL_checkint(l, 2);
    timer->setCompareValue(compare);
    return 0;
}

int CRP6Simulator::luaTimerSetPrescaler(lua_State *l)
{
    NLua::CLuaLocker lualocker;
    CAVRTimer *timer = NLua::checkClassData<CAVRTimer>(l, 1, "timer");
    const int pre = luaL_checkint(l, 2);
    timer->setPrescaler(pre);
    return 0;
}

int CRP6Simulator::luaTimerSetTimeOut(lua_State *l)
{
    NLua::CLuaLocker lualocker;
    CAVRTimer *timer = NLua::checkClassData<CAVRTimer>(l, 1, "timer");

    if (lua_isnumber(l, 2))
        timer->setTimeOutISR(static_cast<EISRTypes>(lua_tointeger(l, 2)));
    else
    {
        lua_pushvalue(l, 2); // Push lua function
        timer->setTimeOutLua();
    }

    // UNDONE: Check arg types
    return 0;
}

int CRP6Simulator::luaTimerIsEnabled(lua_State *l)
{
    NLua::CLuaLocker lualocker;
    CAVRTimer *timer = NLua::checkClassData<CAVRTimer>(l, 1, "timer");
    lua_pushboolean(l, timer->isEnabled());
    return 1;
}

int CRP6Simulator::luaBitIsSet(lua_State *l)
{
    NLua::CLuaLocker lualocker;
    const int data = luaL_checkint(l, 1);
    const int nargs = lua_gettop(l);

    luaL_checktype(l, 2, LUA_TNUMBER);

    bool ret = true;
    for (int i=2; i<=nargs; ++i)
    {
        const int bit = luaL_checkint(l, i);
        if (!(data & (1 << bit)))
        {
            ret = false;
            break;
        }
    }

    lua_pushboolean(l, ret);
    return 1;
}

int CRP6Simulator::luaBitSet(lua_State *l)
{
    NLua::CLuaLocker lualocker;
    int data = luaL_checkint(l, 1);
    const int nargs = lua_gettop(l);

    luaL_checktype(l, 2, LUA_TNUMBER);

    for (int i=2; i<=nargs; ++i)
    {
        const int bit = luaL_checkint(l, i);
        data |= (1 << bit);
    }

    lua_pushinteger(l, data);
    return 1;
}

int CRP6Simulator::luaBitUnSet(lua_State *l)
{
    NLua::CLuaLocker lualocker;
    int data = luaL_checkint(l, 1);
    const int nargs = lua_gettop(l);

    luaL_checktype(l, 2, LUA_TNUMBER);

    for (int i=2; i<=nargs; ++i)
    {
        const int bit = luaL_checkint(l, i);
        data &= ~(1 << bit);
    }

    lua_pushinteger(l, data);
    return 1;
}

int CRP6Simulator::luaBitUnPack(lua_State *l)
{
    NLua::CLuaLocker lualocker;
    const uint32_t low = luaL_checkint(l, 1);
    const uint32_t high = luaL_checkint(l, 2);
    const int size = luaL_checkint(l, 3);

    if (size == 8)
        lua_pushinteger(l, (low & 0xFF) + ((high & 0xFF) << 8));
    else if (size == 16)
        lua_pushinteger(l, (low & 0xFFFF) + ((high & 0xFFFF) << 16));

    return 1;
}

int CRP6Simulator::luaBitAnd(lua_State *l)
{
    NLua::CLuaLocker lualocker;
    const int data = luaL_checkint(l, 1);
    const int bits = luaL_checkint(l, 2);
    lua_pushinteger(l, (data & bits));
    return 1;
}

int CRP6Simulator::luaAppendLogOutput(lua_State *l)
{
    NLua::CLuaLocker lualocker;
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

    // Emit: function called outside main thread
    instance->emit logTextReady(instance->getLogOutput(t, text));
    return 0;
}

int CRP6Simulator::luaAppendSerialOutput(lua_State *l)
{
    NLua::CLuaLocker lualocker;
    QMutexLocker seriallocker(&instance->serialBufferMutex);
    instance->serialTextBuffer += luaL_checkstring(l, 1);
    return 0;
}

void CRP6Simulator::updateClockDisplay(unsigned long hz)
{
    clockDisplay->display(static_cast<double>(hz) / 1000000);
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

void CRP6Simulator::timedUpdate()
{
    QReadLocker iolocker(&IORegisterReadWriteLock);

    for (int i=0; i<IO_END; ++i)
    {
        TIORegisterData d = IORegisterData[i]; // Shortcut
        IORegisterTableWidget->item(i, 0)->setText(QString::number(d));
        IORegisterTableWidget->item(i, 1)->setText(QString::number(d, 16));
        IORegisterTableWidget->item(i, 2)->setText(QString::number(d, 2).rightJustified(16, '0', true));
        IORegisterTableWidget->resizeRowToContents(i);
    }

    IORegisterTableWidget->resizeColumnsToContents();

    QMutexLocker serialocker(&instance->serialBufferMutex);
    if (!serialTextBuffer.isEmpty())
    {
        // Append text, without adding a new paragraph
        QTextCursor cur = serialOutputWidget->textCursor();
        cur.movePosition(QTextCursor::End);
        serialOutputWidget->setTextCursor(cur);
        serialOutputWidget->insertPlainText(serialTextBuffer);
        serialTextBuffer.clear();
    }
}

void CRP6Simulator::runPlugin()
{
    if (pluginMainThread && !pluginMainThread->isRunning())
    {
        // NOTE: AVR clock should be stopped/reset earlier
        AVRClock->start();
        lastPluginDelay.tv_sec = lastPluginDelay.tv_nsec = 0;
        pluginMainThread->start();
    }
}

TIORegisterData CRP6Simulator::getIORegister(EIORegisterTypes type) const
{
    QReadLocker locker(&IORegisterReadWriteLock);
    return IORegisterData[type];
}

void CRP6Simulator::setIORegister(EIORegisterTypes type, TIORegisterData data)
{
    QWriteLocker locker(&IORegisterReadWriteLock);
    IORegisterData[type] = data;
}

void CRP6Simulator::execISR(EISRTypes type)
{
    QMutexLocker lock(&ISRExecMutex);

    if (!ISRsEnabled)
        return;

    if (ISRFailedArray[type])
        return;

    if (!ISRCacheArray[type])
    {
        QLibrary lib("../test/build/libmyrp6.so"); // UNDONE
        QString func = constructISRFunc(type);
        ISRCacheArray[type] = getLibFunc<TISR>(lib, getCString(func));

        if (!ISRCacheArray[type])
        {
            ISRFailedArray[type] = true;
            return;
        }
    }

    ISRCacheArray[type]();
}
