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
QReadWriteLock CRP6Simulator::generalIOReadWriteLock;
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
    AVRClockThread = new QThread(this);

    // UNDONE
//    connect(AVRClockThread, SIGNAL(started()), AVRClock, SLOT(start()));
    connect(AVRClockThread, SIGNAL(finished()), AVRClock, SLOT(stop()));

    AVRClock->moveToThread(AVRClockThread);

    AVRClockThread->start();
}

void CRP6Simulator::addIOHandler(CBaseIOHandler *handler)
{
    IOHandlerList << handler;
    handler->registerHandler(IOHandlerArray);
}

void CRP6Simulator::initIOHandlers()
{
    for (int i=0; i<IO_END; ++i)
        IOHandlerArray[i] = 0;

    addIOHandler(new CUARTHandler(this));
    addIOHandler(new CTimer0Handler(this));
    addIOHandler(new CTimer1Handler(this));
    addIOHandler(new CTimer2Handler(this));
    addIOHandler(new CTimerMaskHandler(this));
}

void CRP6Simulator::setLuaIOTypes()
{
    // Evil macros and stuff
#define SET_LUA_IO(IO) NLua::setVariable(IO_##IO, #IO, "avr")

    // UART
    SET_LUA_IO(UCSRA);
    SET_LUA_IO(UDR);

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
}

void CRP6Simulator::registerLuaClock()
{
    NLua::setVariable(CAVRClock::TIMER_0, "TIMER0", "clock");
    NLua::setVariable(CAVRClock::TIMER_1A, "TIMER1A", "clock");
    NLua::setVariable(CAVRClock::TIMER_2, "TIMER2", "clock");
}

void CRP6Simulator::registerLuaBindings()
{
    NLua::registerFunction(luaGetGeneralIO, "getGeneralIO", "avr");
    NLua::registerFunction(luaSetGeneralIO, "setGeneralIO", "avr");

    NLua::registerFunction(luaEnableTimer, "enableTimer", "clock");
    NLua::registerFunction(luaSetTimerCompareValue, "setCompareValue", "clock");
    NLua::registerFunction(luaSetTimerPrescaler, "setPrescaler", "clock");
}

void CRP6Simulator::initLua()
{
    setLuaIOTypes();
    setLuaAVRConstants();
    registerLuaClock();
    registerLuaBindings();

    NLua::luaInterface.exec();
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
    TSetGeneralIOCallbacks setiocb =
            getLibFunc<TSetGeneralIOCallbacks>(lib, "setGeneralIOCallbacks");

    if (!mainfunc || !setiocb)
        return; // UNDONE

    AVRClock->stop();

    terminatePluginMainThread();

    pluginMainThread = new CCallPluginMainThread(mainfunc, this);

    setiocb(generalIOSetCB, generalIOGetCB);

    for (int i=0; i<IO_END; ++i)
        generalIOData[i] = 0;

    for (int i=0; i<ISR_END; ++i)
    {
        ISRCacheArray[i] = 0;
        ISRFailedArray[i] = false;
    }

    foreach (CBaseIOHandler *handler, IOHandlerList)
    {
        handler->initPlugin();
    }
}

void CRP6Simulator::generalIOSetCB(EGeneralIOTypes type, TGeneralIOData data)
{
    instance->setGeneralIO(type, data);
    if (instance->IOHandlerArray[type])
        instance->IOHandlerArray[type]->handleIOData(type, data);
}

TGeneralIOData CRP6Simulator::generalIOGetCB(EGeneralIOTypes type)
{
    return instance->getGeneralIO(type);
}

int CRP6Simulator::luaGetGeneralIO(lua_State *l)
{
    const EGeneralIOTypes type = static_cast<EGeneralIOTypes>(luaL_checkint(l, 1));
    lua_pushinteger(l, instance->getGeneralIO(type));
    return 1;
}

int CRP6Simulator::luaSetGeneralIO(lua_State *l)
{
    const EGeneralIOTypes type = static_cast<EGeneralIOTypes>(luaL_checkint(l, 1));
    const int data = luaL_checkint(l, 2);
    instance->setGeneralIO(type, data);
    return 0;
}

int CRP6Simulator::luaEnableTimer(lua_State *l)
{
    const CAVRClock::EAVRTimers timer = static_cast<CAVRClock::EAVRTimers>(luaL_checkint(l, 1));
    const bool e = NLua::checkBoolean(2);
    instance->AVRClock->enableTimer(timer, e);
    return 0;
}

int CRP6Simulator::luaSetTimerCompareValue(lua_State *l)
{
    const CAVRClock::EAVRTimers timer = static_cast<CAVRClock::EAVRTimers>(luaL_checkint(l, 1));
    const int compare = luaL_checkint(l, 2);
    instance->AVRClock->getTimer(timer)->setCompareValue(compare);
    return 0;
}

int CRP6Simulator::luaSetTimerPrescaler(lua_State *l)
{
    const CAVRClock::EAVRTimers timer = static_cast<CAVRClock::EAVRTimers>(luaL_checkint(l, 1));
    const int pre = luaL_checkint(l, 2);
    instance->AVRClock->getTimer(timer)->setCompareValue(pre);
    return 0;
}

void CRP6Simulator::runPlugin()
{
    if (pluginMainThread && !pluginMainThread->isRunning())
    {
        // NOTE: AVR clock should be stopped earlier
        AVRClock->reset();
        AVRClock->start();
        pluginMainThread->start();
    }
}

TGeneralIOData CRP6Simulator::getGeneralIO(EGeneralIOTypes type) const
{
    QReadLocker locker(&generalIOReadWriteLock);
    return generalIOData[type];
}

void CRP6Simulator::setGeneralIO(EGeneralIOTypes type, TGeneralIOData data)
{
    QWriteLocker locker(&generalIOReadWriteLock);
    generalIOData[type] = data;
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
