#include "simulator.h"
#include "avrtimer.h"
#include "pluginthread.h"
#include "projectsettings.h"
#include "rp6simul.h"
#include "utils.h"

#include <signal.h>
#include <unistd.h>

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

#include <QApplication>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSettings>
#include <QTime>

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

    // SPI
    "SPCR",
    "SPSR",
    "SPDR",

    // TWI
    "TWBR",
    "TWCR",
    "TWSR",
    "TWDR",
    "TWAR",
};


namespace {

static int luaPanicCallback(lua_State *l)
{
    // UNDONE (might not be a gui thread)
//    QMessageBox::critical(CRP6Simulator::getInstance(), "Lua error",
//                          lua_tostring(l, 1));
    qWarning() << "Lua error:" << lua_tostring(l, -1);
    Q_ASSERT(false);
    return 0;
}

inline CSimulator *getInstanceFromData(void *d)
{
    return reinterpret_cast<CSimulator *>(d);
}

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
    case ISR_INT0_vect: f = "INT0_vect"; break;
    case ISR_INT1_vect: f = "INT1_vect"; break;
    case ISR_INT2_vect: f = "INT2_vect"; break;
    case ISR_TWI_vect: f = "TWI_vect"; break;
    default: Q_ASSERT(false); break;
    }

    return "ISR_" + f;
}

QList<QVariant> packLuaTWIData(lua_State *l, int start, int end)
{
    QList<QVariant> ret;

    for (int i=start; i<=end; ++i)
    {
        switch (lua_type(l, i))
        {
        case LUA_TNUMBER: ret << (double)lua_tonumber(l, i); break;
        case LUA_TBOOLEAN: ret << (bool)lua_toboolean(l, i); break;
        case LUA_TSTRING: ret << QString(lua_tostring(l, i)); break;
        case LUA_TNIL: /* nothing */ break;
        default: qFatal("Unsupported TWI arg type\n"); break;
        }
    }

    return ret;
}

void pushPackedLuaTWIData(lua_State *l, const QList<QVariant> &list)
{
    foreach (QVariant v, list)
    {
        // Limit to double, boolean and string
        switch (v.type())
        {
        case QVariant::Double: lua_pushnumber(l, v.toDouble()); break;
        case QVariant::Bool: lua_pushboolean(l, v.toBool()); break;
        case QVariant::String: lua_pushstring(l, qPrintable(v.toString())); break;
        default: Q_ASSERT(false); break;
        }
    }
}

}


CSimulator::CSimulator(QObject *parent) :
    QObject(parent), pluginMainThread(0), quitPlugin(false),
    ISRExecMutex(QMutex::Recursive), luaTWIHandler(0)
{
    initLua();
    initAVRClock();
}

CSimulator::~CSimulator()
{
    if (luaState)
    {
        // Debug
        qDebug("Lua stack:\n");
        NLua::stackDump(luaState);

        lua_close(luaState);
        luaState = 0;
    }

    terminateAVRClock();
    delete AVRClock;
    terminatePluginMainThread();
}

void CSimulator::setLuaIOTypes()
{
    for (int i=0; i<IO_END; ++i)
    {
        QString io = QString("IO_%1").arg(IORegisterStringArray[i]);
        NLua::setVariable(luaState, i, qPrintable(io), "avr");
    }
}

void CSimulator::setLuaAVRConstants()
{
    // Evil macros and stuff
#define SET_LUA_CONSTANT(C) NLua::setVariable(luaState, C, #C, "avr")

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

    // SPI
    SET_LUA_CONSTANT(SPIF);
    SET_LUA_CONSTANT(WCOL);
    SET_LUA_CONSTANT(SPI2X);
    SET_LUA_CONSTANT(SPIE);
    SET_LUA_CONSTANT(SPE);
    SET_LUA_CONSTANT(DORD);
    SET_LUA_CONSTANT(MSTR);
    SET_LUA_CONSTANT(CPOL);
    SET_LUA_CONSTANT(CPHA);
    SET_LUA_CONSTANT(SPR1);
    SET_LUA_CONSTANT(SPR0);

     // TWI
    SET_LUA_CONSTANT(TWINT);
    SET_LUA_CONSTANT(TWEA);
    SET_LUA_CONSTANT(TWSTA);
    SET_LUA_CONSTANT(TWSTO);
    SET_LUA_CONSTANT(TWWC);
    SET_LUA_CONSTANT(TWEN);
    SET_LUA_CONSTANT(TWIE);
    SET_LUA_CONSTANT(TWA6);
    SET_LUA_CONSTANT(TWA5);
    SET_LUA_CONSTANT(TWA4);
    SET_LUA_CONSTANT(TWA3);
    SET_LUA_CONSTANT(TWA2);
    SET_LUA_CONSTANT(TWA1);
    SET_LUA_CONSTANT(TWA0);
    SET_LUA_CONSTANT(TWGCE);
    SET_LUA_CONSTANT(TWS7);
    SET_LUA_CONSTANT(TWS6);
    SET_LUA_CONSTANT(TWS5);
    SET_LUA_CONSTANT(TWS4);
    SET_LUA_CONSTANT(TWS3);
    SET_LUA_CONSTANT(TWPS1);
    SET_LUA_CONSTANT(TWPS0);

    // ISRs
    SET_LUA_CONSTANT(ISR_USART_RXC_vect);
    SET_LUA_CONSTANT(ISR_TIMER0_COMP_vect);
    SET_LUA_CONSTANT(ISR_TIMER1_COMPA_vect);
    SET_LUA_CONSTANT(ISR_TIMER2_COMP_vect);
    SET_LUA_CONSTANT(ISR_INT0_vect);
    SET_LUA_CONSTANT(ISR_INT1_vect);
    SET_LUA_CONSTANT(ISR_INT2_vect);
    SET_LUA_CONSTANT(ISR_TWI_vect);

#undef SET_LUA_CONSTANT
}

void CSimulator::initLua()
{
    luaState = lua_open();

    if (!luaState)
        qFatal("Could not open lua VM\n");

    NLua::addLuaLockState(luaState);

    luaL_openlibs(luaState);
    lua_atpanic(luaState, luaPanicCallback);
    setLuaIOTypes();
    setLuaAVRConstants();

    // avr
    NLua::registerFunction(luaState, luaAvrGetIORegister, "getIORegister",
                           "avr", this);
    NLua::registerFunction(luaState, luaAvrSetIORegister, "setIORegister",
                           "avr", this);
    NLua::registerFunction(luaState, luaAvrSetIORegisterIgnoreEqual,
                           "setIORegisterIgnoreEqual", "avr", this);
    NLua::registerFunction(luaState, luaAvrExecISR, "execISR", "avr", this);
    NLua::registerFunction(luaState, luaAvrSetTWIMSGHandler, "setTWIMSGHandler",
                           "avr", this);
    NLua::registerFunction(luaState, luaAvrSendTWIMSG, "sendTWIMSG", "avr", this);

    // clock
    NLua::registerFunction(luaState, luaClockSetTargetSpeed, "setTargetSpeed",
                           "clock", this);
    NLua::registerFunction(luaState, luaClockCreateTimer, "createTimer",
                           "clock", this);
    NLua::registerFunction(luaState, luaClockEnableTimer, "enableTimer",
                           "clock", this);

    // timer class
    NLua::registerClassFunction(luaState, luaTimerSetCompareValue,
                                "setCompareValue", "timer");
    NLua::registerClassFunction(luaState, luaTimerSetPrescaler, "setPrescaler",
                                "timer");
    NLua::registerClassFunction(luaState, luaTimerSetTimeOut, "setTimeOut",
                                "timer", this);
    NLua::registerClassFunction(luaState, luaTimerIsEnabled, "isEnabled",
                                "timer");

    // bit
    NLua::registerFunction(luaState, luaBitIsSet, "isSet", "bit");
    NLua::registerFunction(luaState, luaBitSet, "set", "bit");
    NLua::registerFunction(luaState, luaBitUnSet, "unSet", "bit");
    NLua::registerFunction(luaState, luaBitUnPack, "unPack", "bit");
    NLua::registerFunction(luaState, luaBitLower, "lower", "bit");
    NLua::registerFunction(luaState, luaBitUpper, "upper", "bit");
    NLua::registerFunction(luaState, luaBitAnd, "bitAnd", "bit");
    NLua::registerFunction(luaState, luaBitOr, "bitOr", "bit");
    NLua::registerFunction(luaState, luaBitXor, "bitXor", "bit");
    NLua::registerFunction(luaState, luaBitShiftLeft, "shiftLeft", "bit");
    NLua::registerFunction(luaState, luaBitShiftRight, "shiftRight", "bit");
}

void CSimulator::initAVRClock()
{
    // From http://labs.qt.nokia.com/2010/06/17/youre-doing-it-wrong/

    AVRClock = new CAVRClock;
    AVRClockThread = new QThread(this);
    connect(AVRClockThread, SIGNAL(finished()), AVRClock, SLOT(stop()));
    AVRClock->moveToThread(AVRClockThread);
    AVRClockThread->start();
}

void CSimulator::terminateAVRClock()
{
    qDebug() << "Terminating AVR clock thread";
    AVRClockThread->quit();
    AVRClockThread->wait();
}

void CSimulator::terminatePluginMainThread()
{
    if (pluginMainThread)
    {
        const int steptime = 1500;
        const int steps = 3;

        QProgressDialog prgdialog(CRP6Simulator::getInstance(), Qt::FramelessWindowHint);
        prgdialog.setWindowTitle("Terminating RP6 plugin...");
        prgdialog.setRange(1, steps * steptime);
        prgdialog.setMinimumDuration(250);
        prgdialog.setValue(0);
        prgdialog.setCancelButton(0); // No canceling

        prgdialog.setLabelText("Asking RP6 plugin nicely to quit...");
        quitPlugin = true;

        int step = 1;
        QTime time;
        time.start();
        while (true)
        {
            if (pluginMainThread->wait(10))
                break;

            const int el = time.elapsed();

            prgdialog.setValue(el + ((step-1) * steptime));

            if ((el < 0) || (el > 1500))
            {
                ++step;
                if (step == 2)
                {
                    prgdialog.setLabelText("Asking not so nicely to quit...");
                    pluginMainThread->terminate();
                    time.restart();
                }
                else if (step == 3)
                {
                    prgdialog.setLabelText("Plugin seems to be stuck, not asking anymore!");
                    // UNDONE: Winies support
#ifndef Q_OS_WIN32
                    sigval sv;
                    pthread_sigqueue(pluginMainThread->getThreadID(),
                                     SIGUSR1, sv);
#endif
                    time.restart();
                }
                else
                {
                    QMessageBox::warning(CRP6Simulator::getInstance(), "Terminate plugin error",
                                         "Failed to terminate plugin!\n"
                                         "It's best to restart to program "
                                         "before re-running to avoid "
                                         "undefined behaviour.");
                    break;
                }
            }

            qApp->processEvents();
        }

        delete pluginMainThread;
        pluginMainThread = 0;
        quitPlugin = false;
    }
}

bool CSimulator::initPlugin()
{
    if (RP6Plugin.isLoaded() && !RP6Plugin.unload())
    {
        QMessageBox::critical(CRP6Simulator::getInstance(), "Unload plugin error",
                              "Could not unload plugin file!\n"
                              "This may result in undefined behaviour"
                              "when simulation is started. It's best"
                              "to restart the application.");
        return false;
    }

    if (currentPluginFileName.isEmpty())
    {
        QMessageBox::critical(CRP6Simulator::getInstance(), "Plugin error", "Project has no RP6 plugin set");
        return false;
    }

    RP6Plugin.setFileName(currentPluginFileName);

    if (!RP6Plugin.load())
    {
        QMessageBox::critical(CRP6Simulator::getInstance(), "Plugin error",
                              "Failed to load plugin:\n" + RP6Plugin.errorString());
        return false;
    }

    TCallPluginMainFunc mainfunc =
            getLibFunc<TCallPluginMainFunc>(RP6Plugin, "callAvrMain");
    TSetPluginCallbacks setplugincb =
            getLibFunc<TSetPluginCallbacks>(RP6Plugin, "setPluginCallbacks");

    if (!mainfunc || !setplugincb)
    {
        QMessageBox::critical(CRP6Simulator::getInstance(), "Plugin error",
                              "Could not resolve mandatory symbols.\n"
                              "Incorrect plugin specified?");
        return false;
    }

    setplugincb(IORegisterSetCB, IORegisterGetCB, enableISRsCB, this);

    lua_getglobal(luaState, "initPlugin");

    // Push driver list
    if (!currentDriverList.isEmpty())
    {
        NLua::pushStringList(luaState, currentDriverList);
        lua_call(luaState, 1, 0);
    }
    else
    {
        qDebug() << "Running without any drivers!";
        lua_call(luaState, 0, 0);
    }

    Q_ASSERT(!pluginMainThread);
    pluginMainThread = new CCallPluginMainThread(mainfunc, this);

    return true;
}

void CSimulator::checkPluginThreadDelay()
{
    // This function is called whenever register data is accessed (get/set)
    // from the RP6 plugin thread. It adds a small delay (nanosleep) within small
    // intervals to prevent constant CPU hammering by the plugin. This
    // obviously will only work when the RP6 program accesses IO, however this
    // is commonly done. Furthermore it will also check if the user requested
    // to stop the plugin

    // Plugin thread running? (ISRs may also call this function which do not
    // have to be delayed)
    if (QThread::currentThread() == pluginMainThread)
    {
        if (quitPlugin)
        {
#ifdef Q_OS_WIN32
            ExitThread(0);
#else
            pthread_exit(0);
#endif
            return;
        }

        // Init?
        if (!lastPluginDelay.tv_sec && !lastPluginDelay.tv_nsec)
            clock_gettime(CLOCK_MONOTONIC, &lastPluginDelay);
        else
        {
            timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            const unsigned long delta = getUSDiff(lastPluginDelay, ts);

            if (delta > 50) // Delay every 50 us
            {
                lastPluginDelay = ts;
#ifdef Q_OS_WIN32
                usleep(1);
#else
                ts.tv_sec = 0; ts.tv_nsec = 1000;
                nanosleep(&ts, 0);
#endif
            }
        }
    }
}

TIORegisterData CSimulator::getIORegister(EIORegisterTypes type) const
{
#ifndef USEATOMIC
    QReadLocker locker(&IORegisterReadWriteLock);
#endif
    return IORegisterData[type];
}

void CSimulator::setIORegister(EIORegisterTypes type, TIORegisterData data)
{
#ifndef USEATOMIC
    QWriteLocker locker(&IORegisterReadWriteLock);
#endif
    IORegisterData[type] = data;
}

void CSimulator::IORegisterSetCB(EIORegisterTypes type, TIORegisterData value,
                                 void *data)
{
    CSimulator *instance = getInstanceFromData(data);
    if (instance->IORegisterDataIgnoreEqual[type] ||
        (instance->getIORegister(type) != value))
    {
        instance->setIORegister(type, value);

        NLua::CLuaLocker lualocker(instance->luaState);
        lua_getglobal(instance->luaState, "handleIOData");
        lua_pushinteger(instance->luaState, type);
        lua_pushinteger(instance->luaState, value);

        lua_call(instance->luaState, 2, 0);

        lualocker.unlock();
    }

    instance->checkPluginThreadDelay();
}

TIORegisterData CSimulator::IORegisterGetCB(EIORegisterTypes type, void *data)
{
    CSimulator *instance = getInstanceFromData(data);
    instance->checkPluginThreadDelay();
    return instance->getIORegister(type);
}

void CSimulator::enableISRsCB(bool e, void *data)
{
    CSimulator *instance = getInstanceFromData(data);
    QMutexLocker lock(&instance->ISRExecMutex);
    instance->ISRsEnabled = e;
}

void CSimulator::timeOutCallback(CAVRTimer *timer, void *data)
{
    CSimulator *instance = getInstanceFromData(data);
    QMap<CAVRTimer *, STimeOutInfo>::iterator it =
            instance->timeOutMap.find(timer);
    Q_ASSERT(it != instance->timeOutMap.end());

    if (it != instance->timeOutMap.end())
    {
        if (it->timeOutType == STimeOutInfo::TIMEOUT_LUA)
        {
            NLua::CLuaLocker lualocker(instance->luaState);
            lua_rawgeti(instance->luaState, LUA_REGISTRYINDEX, it->luaRef);
            lua_call(instance->luaState, 0, 0); // UNDONE: error handling
        }
        else
            instance->execISR(it->ISRType);
    }
}

int CSimulator::luaAvrGetIORegister(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    CSimulator *instance = NLua::getFromClosure<CSimulator *>(l);
    const EIORegisterTypes type = static_cast<EIORegisterTypes>(luaL_checkint(l, 1));
    lua_pushinteger(l, instance->getIORegister(type));
    return 1;
}

int CSimulator::luaAvrSetIORegister(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    CSimulator *instance = NLua::getFromClosure<CSimulator *>(l);
    const EIORegisterTypes type = static_cast<EIORegisterTypes>(luaL_checkint(l, 1));
    const int data = luaL_checkint(l, 2);
    instance->setIORegister(type, data);
    return 0;
}

int CSimulator::luaAvrSetIORegisterIgnoreEqual(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    CSimulator *instance = NLua::getFromClosure<CSimulator *>(l);
    const EIORegisterTypes type = static_cast<EIORegisterTypes>(luaL_checkint(l, 1));
    instance->IORegisterDataIgnoreEqual[type] = NLua::checkBoolean(l, 2);
    return 0;
}

int CSimulator::luaAvrExecISR(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    CSimulator *instance = NLua::getFromClosure<CSimulator *>(l);
    const EISRTypes isr = static_cast<EISRTypes>(luaL_checkint(l, 1));
    instance->execISR(isr);
    return 0;
}

int CSimulator::luaAvrSetTWIMSGHandler(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    CSimulator *instance = NLua::getFromClosure<CSimulator *>(l);
    luaL_checktype(l, 1, LUA_TFUNCTION);
    lua_pushvalue(l, 1);

    QMutexLocker twilock(&instance->luaTWIHandlerMutex);
    if (instance->luaTWIHandler != 0)
        luaL_unref(l, LUA_REGISTRYINDEX, instance->luaTWIHandler);
    instance->luaTWIHandler = luaL_ref(l, LUA_REGISTRYINDEX);
    return 0;
}

int CSimulator::luaAvrSendTWIMSG(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    CSimulator *instance = NLua::getFromClosure<CSimulator *>(l);

    const char *msg = luaL_checkstring(l, 1);
    const int nargs = lua_gettop(l);
    QList<QVariant> args(packLuaTWIData(l, 2, nargs));

    instance->emit luaTWIMSGSend(msg, args);

    return 0;
}

int CSimulator::luaClockSetTargetSpeed(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    CSimulator *instance = NLua::getFromClosure<CSimulator *>(l);
    instance->AVRClock->setTargetClockSpeed(luaL_checkint(l, 1));
    return 0;
}

int CSimulator::luaClockCreateTimer(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    CSimulator *instance = NLua::getFromClosure<CSimulator *>(l);
    CAVRTimer *timer = instance->AVRClock->createTimer(timeOutCallback,
                                                       instance);
    NLua::createClass(l, timer, "timer", luaTimerDestr, instance);
    return 1;
}

int CSimulator::luaClockEnableTimer(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    CSimulator *instance = NLua::getFromClosure<CSimulator *>(l);
    CAVRTimer *timer = NLua::checkClassData<CAVRTimer>(l, 1, "timer");
    const bool e = NLua::checkBoolean(l, 2);
    instance->AVRClock->enableTimer(timer, e);
    return 0;
}

int CSimulator::luaTimerDestr(lua_State *l)
{
    qDebug() << "Removing timer";

    // NLua::CLuaLocker lualocker(l);
    CSimulator *instance = NLua::getFromClosure<CSimulator *>(l);
    CAVRTimer *timer = NLua::checkClassData<CAVRTimer>(l, 1, "timer");
    instance->AVRClock->removeTimer(timer);

    QMap<CAVRTimer *, STimeOutInfo>::iterator it =
            instance->timeOutMap.find(timer);
    if (it != instance->timeOutMap.end())
    {
        if (it->timeOutType == STimeOutInfo::TIMEOUT_LUA)
            luaL_unref(instance->luaState, LUA_REGISTRYINDEX, it->luaRef);
        instance->timeOutMap.erase(it);
    }

    delete timer;
    return 0;
}

int CSimulator::luaTimerSetCompareValue(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    CAVRTimer *timer = NLua::checkClassData<CAVRTimer>(l, 1, "timer");
    const int compare = luaL_checkint(l, 2);
    timer->setCompareValue(compare);
    return 0;
}

int CSimulator::luaTimerSetPrescaler(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    CAVRTimer *timer = NLua::checkClassData<CAVRTimer>(l, 1, "timer");
    const int pre = luaL_checkint(l, 2);
    timer->setPrescaler(pre);
    return 0;
}

int CSimulator::luaTimerSetTimeOut(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    CSimulator *instance = NLua::getFromClosure<CSimulator *>(l);
    CAVRTimer *timer = NLua::checkClassData<CAVRTimer>(l, 1, "timer");

    // UNDONE
    if (timer->isEnabled())
        qFatal("Tried to change timer timeout function when being active.\n");

    QMap<CAVRTimer *, STimeOutInfo>::iterator it =
            instance->timeOutMap.find(timer);
    if (it != instance->timeOutMap.end())
    {
        if (it->timeOutType == STimeOutInfo::TIMEOUT_LUA)
            luaL_unref(l, LUA_REGISTRYINDEX, it->luaRef);
    }

    if (lua_isnumber(l, 2))
        instance->timeOutMap[timer] =
                STimeOutInfo(static_cast<EISRTypes>(lua_tointeger(l, 2)));
    else if (lua_isfunction(l, 2))
    {
        lua_pushvalue(l, 2); // Push lua function
        instance->timeOutMap[timer] =
                STimeOutInfo(luaL_ref(l, LUA_REGISTRYINDEX));
    }
    else
        luaL_argerror(l, 2, "Wrong timeout argument: needs ISR or function");

    return 0;
}

int CSimulator::luaTimerIsEnabled(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    CAVRTimer *timer = NLua::checkClassData<CAVRTimer>(l, 1, "timer");
    lua_pushboolean(l, timer->isEnabled());
    return 1;
}

int CSimulator::luaBitIsSet(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
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

int CSimulator::luaBitSet(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
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

int CSimulator::luaBitUnSet(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
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

int CSimulator::luaBitUnPack(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    const uint32_t low = luaL_checkint(l, 1);
    const uint32_t high = luaL_checkint(l, 2);
    const int size = luaL_checkint(l, 3);

    if (size == 8)
        lua_pushinteger(l, (low & 0xFF) | ((high & 0xFF) << 8));
    else if (size == 16)
        lua_pushinteger(l, (low & 0xFFFF) | ((high & 0xFFFF) << 16));
    else
        luaL_argerror(l, 3, "unPack size should be 8 or 16");

    return 1;
}

int CSimulator::luaBitLower(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    const uint32_t value = luaL_checkint(l, 1);
    const int size = luaL_checkint(l, 2);

    if (size == 8)
        lua_pushinteger(l, value & 0X00FF);
    else if (size == 16)
        lua_pushinteger(l, value & 0XFFFF);

    return 1;
}

int CSimulator::luaBitUpper(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    const uint32_t value = luaL_checkint(l, 1);
    const int size = luaL_checkint(l, 2);

    if (size == 8)
        lua_pushinteger(l, (value & 0XFF00) >> 8);
    else if (size == 16)
        lua_pushinteger(l, (value & 0XFFFF0000) >> 16);

    return 1;
}

int CSimulator::luaBitAnd(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    const int data = luaL_checkint(l, 1);
    const int bits = luaL_checkint(l, 2);
    lua_pushinteger(l, (data & bits));
    return 1;
}

int CSimulator::luaBitOr(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    const int data = luaL_checkint(l, 1);
    const int bits = luaL_checkint(l, 2);
    lua_pushinteger(l, (data | bits));
    return 1;
}

int CSimulator::luaBitXor(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    const int data = luaL_checkint(l, 1);
    const int bits = luaL_checkint(l, 2);
    lua_pushinteger(l, (data ^ bits));
    return 1;
}

int CSimulator::luaBitShiftLeft(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    const int data = luaL_checkint(l, 1);
    const int bits = luaL_checkint(l, 2);
    lua_pushinteger(l, (data << bits));
    return 1;
}

int CSimulator::luaBitShiftRight(lua_State *l)
{
    // NLua::CLuaLocker lualocker(l);
    const int data = luaL_checkint(l, 1);
    const int bits = luaL_checkint(l, 2);
    lua_pushinteger(l, (data >> bits));
    return 1;
}

void CSimulator::handleLuaTWIMSG(const QString &msg, const QList<QVariant> &args)
{
    // UNDONE: Do we actually need a mutex for this at all?
    QMutexLocker twilocker(&luaTWIHandlerMutex);
    const int handlerref = luaTWIHandler;
    twilocker.unlock();

    if (luaTWIHandler == 0)
        return;

    NLua::CLuaLocker lualocker(luaState);
    lua_rawgeti(luaState, LUA_REGISTRYINDEX, handlerref);
    lua_pushstring(luaState, qPrintable(msg));
    pushPackedLuaTWIData(luaState, args);
    lua_call(luaState, 1 + args.size(), 0);
}

void CSimulator::startLua(const char *name)
{
    const QString p = QDir::toNativeSeparators("lua/main.lua");
    if (luaL_dofile(luaState, qPrintable(p)))
        NLua::luaError(luaState, true);

    lua_getglobal(luaState, "init");
    lua_pushstring(luaState, name);
    lua_call(luaState, 1, 0);
}

bool CSimulator::loadProjectFile(const QSettings &settings)
{
    const QString plugfile = settings.value("plugin").toString();
    if (!verifyPluginFile(plugfile))
        return false;

    currentPluginFileName = plugfile;
    currentDriverList = settings.value("drivers").toStringList();

    if (currentDriverList.isEmpty())
        QMessageBox::warning(CRP6Simulator::getInstance(), "Empty driverlist",
                             "Project does not have any drivers specified!");

    stopPlugin();
    return true;
}

void CSimulator::execISR(EISRTypes type)
{
    QMutexLocker lock(&ISRExecMutex);

    if (!ISRsEnabled)
        return;

    if (ISRFailedArray[type])
        return;

    if (!ISRCacheArray[type])
    {
        QString func = constructISRFunc(type);
        ISRCacheArray[type] = getLibFunc<TISR>(RP6Plugin, qPrintable(func));

        if (!ISRCacheArray[type])
        {
            ISRFailedArray[type] = true;
            return;
        }
    }

    // Store func to avoid thread locks
    TISR isr = ISRCacheArray[type];
    lock.unlock();
    isr();
}

bool CSimulator::runPlugin()
{
    if (!initPlugin())
        return false;

    Q_ASSERT(!pluginMainThread->isRunning());

    // NOTE: AVR clock should be stopped earlier
    AVRClock->start();
    lastPluginDelay.tv_sec = lastPluginDelay.tv_nsec = 0;
    pluginMainThread->start();

    return true;
}

void CSimulator::stopPlugin()
{
    terminatePluginMainThread();

    // Stop clock after terminating the thread: the plugin may be
    // sleeping and thus would enter an infinite loop
    AVRClock->stop();

    // Wait untill clock is really stopped.
    // UNDONE: Thread safe?
    while (AVRClock->isActive())
        ;

    QMutexLocker twilock(&luaTWIHandlerMutex);
    if (luaTWIHandler != 0)
    {
        luaL_unref(luaState, LUA_REGISTRYINDEX, luaTWIHandler);
        luaTWIHandler = 0;
    }

    lua_getglobal(luaState, "closePlugin");
    lua_call(luaState, 0, 0);

    for (int i=0; i<IO_END; ++i)
    {
        IORegisterData[i] = 0;
        IORegisterDataIgnoreEqual[i] = false;
    }

    ISRsEnabled = false;
    for (int i=0; i<ISR_END; ++i)
    {
        ISRCacheArray[i] = 0;
        ISRFailedArray[i] = false;
    }
}
