#include "rp6simul.h"
#include "pluginthread.h"
#include "iohandler.h"
#include "avrtimer.h"
#include "lua.h"

#include <QtGui>
#include <QLibrary>

namespace {

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

}

CRP6Simulator *CRP6Simulator::instance = 0;
QReadWriteLock CRP6Simulator::IORegisterReadWriteLock;
QMutex CRP6Simulator::ISRExecMutex;

CRP6Simulator::CRP6Simulator(QWidget *parent) : QMainWindow(parent),
    pluginMainThread(0)
{
    Q_ASSERT(!instance);
    instance = this;

    QWidget *cw = new QWidget(this);
    setCentralWidget(cw);

    QVBoxLayout *vbox = new QVBoxLayout(cw);

    QPushButton *button = new QPushButton("Start");
    connect(button, SIGNAL(clicked()), this, SLOT(runPlugin()));
    vbox->addWidget(button);

    vbox->addWidget(clockDisplay = new QLCDNumber(4));

    vbox->addWidget(logWidget = new QPlainTextEdit);
    logWidget->setReadOnly(true);
    logWidget->setCenterOnScroll(true);
    connect(this, SIGNAL(logTextReady(QString)), logWidget,
            SLOT(appendHtml(QString)));

    initAVRClock();
    initIOHandlers();
    initLua();
    initPlugin();
}

CRP6Simulator::~CRP6Simulator()
{
    terminateAVRClock();
    delete AVRClock;
    terminatePluginMainThread();
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

void CRP6Simulator::addIOHandler(CBaseIOHandler *handler)
{
#if 0
    IOHandlerList << handler;
    handler->registerHandler(IOHandlerArray);
#endif
}

void CRP6Simulator::initIOHandlers()
{
    for (int i=0; i<IO_END; ++i)
        IOHandlerArray[i] = 0;

#if 0
    addIOHandler(new CUARTHandler(this));
    addIOHandler(new CTimer0Handler(this));
    addIOHandler(new CTimer1Handler(this));
    addIOHandler(new CTimer2Handler(this));
    addIOHandler(new CTimerMaskHandler(this));
#endif
}

void CRP6Simulator::setLuaIOTypes()
{
    // Evil macros and stuff
#define SET_LUA_IO(IO) NLua::setVariable(IO_##IO, "IO_"#IO, "avr")

    // Timer0
    SET_LUA_IO(TCCR0);
    SET_LUA_IO(OCR0);

    // Timer1
    SET_LUA_IO(TCCR1A);
    SET_LUA_IO(TCCR1B);
    SET_LUA_IO(OCR1A);
    SET_LUA_IO(OCR1B);
    SET_LUA_IO(OCR1AL);
    SET_LUA_IO(OCR1AH);
    SET_LUA_IO(OCR1BL);
    SET_LUA_IO(OCR1BH);
    SET_LUA_IO(ICR1);

    // Timer2
    SET_LUA_IO(TCCR2);
    SET_LUA_IO(OCR2);

    // TIMSK
    SET_LUA_IO(TIMSK);

    // UART
    SET_LUA_IO(UCSRA);
    SET_LUA_IO(UCSRB);
    SET_LUA_IO(UCSRC);
    SET_LUA_IO(UDR);
    SET_LUA_IO(UBRR);
    SET_LUA_IO(UBRRL);
    SET_LUA_IO(UBRRH);

#undef SET_LUA_IO
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

    // global
    NLua::registerFunction(luaAppendLogOutput, "appendLogOutput");

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

void CRP6Simulator::initPlugin()
{
    // UNDONE: unload previous library (? probably only name is cached)

    QLibrary lib("../avrglue/libmyrp6.so"); // UNDONE

    if (!lib.load())
    {
        QMessageBox::critical(this, "Error",
                              "Failed to load library:\n" + lib.errorString());
        return;
    }

    TCallPluginMainFunc mainfunc =
            getLibFunc<TCallPluginMainFunc>(lib, "callAvrMain");
    TSetIORegisterCallbacks setiocb =
            getLibFunc<TSetIORegisterCallbacks>(lib, "setIORegisterCallbacks");

    if (!mainfunc || !setiocb)
        return; // UNDONE

    AVRClock->stop();
    AVRClock->reset();

    terminatePluginMainThread();

    pluginMainThread = new CCallPluginMainThread(mainfunc, this);

    setiocb(IORegisterSetCB, IORegisterGetCB);

    for (int i=0; i<IO_END; ++i)
        IORegisterData[i] = 0;

    for (int i=0; i<ISR_END; ++i)
    {
        ISRCacheArray[i] = 0;
        ISRFailedArray[i] = false;
    }

    lua_getglobal(NLua::luaInterface, "initPlugin");
    lua_call(NLua::luaInterface, 0, 0);

#if 0
    foreach (CBaseIOHandler *handler, IOHandlerList)
    {
        handler->initPlugin();
    }
#endif
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

QString CRP6Simulator::getLogOutput(ELogType type, const QString &text) const
{
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
#if 0
    if (instance->IOHandlerArray[type])
        instance->IOHandlerArray[type]->handleIOData(type, data);
#endif
}

TIORegisterData CRP6Simulator::IORegisterGetCB(EIORegisterTypes type)
{
    instance->checkPluginThreadDelay();
    return instance->getIORegister(type);
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
    NLua::CLuaLocker lualocker;
    delete NLua::checkClassData<CAVRTimer>(l, 1, "timer");
    qDebug() << "Removing timer";
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

    // Ugh, I suck at 'bit math'
    if (size == 8)
        lua_pushinteger(l, (low & 0xF0) + (high & 0x0F));
    if (size == 16)
        lua_pushinteger(l, (low & 0xFF00) + (high & 0x00FF));
    else if (size == 32)
        lua_pushinteger(l, (low & 0xFFFF0000) + (high & 0x0000FFFF));
    else
        ; // UNDONE

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

void CRP6Simulator::updateClockDisplay(unsigned long hz)
{
    clockDisplay->display(static_cast<double>(hz) / 1000000);
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
    // UNDONE: Renable/disable ISR's (cli()/sei())

    QMutexLocker lock(&ISRExecMutex);

    if (ISRFailedArray[type])
        return;

    if (!ISRCacheArray[type])
    {
        QLibrary lib("../avrglue/libmyrp6.so"); // UNDONE
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
