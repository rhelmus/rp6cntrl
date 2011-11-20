#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "clock.h"
#include "lua.h"
#include "shared.h"

#include <time.h>
#include <sys/time.h>

#include <QLibrary>
#include <QMutex>
#include <QObject>
#include <QReadWriteLock>
#include <QStringList>

typedef void (*TCallPluginMainFunc)(void);

enum ESimulatorCPUUsage { CPU_LOW=0, CPU_NORMAL, CPU_MAX };

enum EISRTypes
{
    // UART
    ISR_USART_RXC_vect=0,

    // Timers
    ISR_TIMER0_COMP_vect,
    ISR_TIMER1_COMPA_vect,
    ISR_TIMER2_COMP_vect,

    // Encoders
    ISR_INT0_vect,
    ISR_INT1_vect,

    // IR receiver
    ISR_INT2_vect,

    // TWI
    ISR_TWI_vect,

    ISR_END
};

class QSettings;

class CAVRClock;
class CAVRTimer;
class CCallPluginMainThread;

class CSimulator : public QObject
{
    Q_OBJECT

    struct STimeOutInfo
    {
        enum ETimeOutType { TIMEOUT_ISR, TIMEOUT_LUA };
        ETimeOutType timeOutType;
        EISRTypes ISRType;
        int luaRef;
        STimeOutInfo(void) { } // For QMap
        STimeOutInfo(EISRTypes i) : timeOutType(TIMEOUT_ISR), ISRType(i) { }
        STimeOutInfo(int l) : timeOutType(TIMEOUT_LUA), luaRef(l) { }
    };

    lua_State *luaState;
    CAVRClock *AVRClock;
    QThread *AVRClockThread;
    QMap<CAVRTimer *, STimeOutInfo> timeOutMap;

    CCallPluginMainThread *pluginMainThread;
    timespec lastPluginDelay;
    volatile bool quitPlugin;

#if USEATOMIC
    QAtomicInt IORegisterData[IO_END];
#else
    TIORegisterData IORegisterData[IO_END];
    mutable QReadWriteLock IORegisterReadWriteLock;
#endif
    bool IORegisterDataIgnoreEqual[IO_END];

    bool ISRsEnabled;
    typedef void (*TISR)(void);
    TISR ISRCacheArray[ISR_END];
    bool ISRFailedArray[ISR_END];
    QMutex ISRExecMutex;
    int luaTWIHandler;

    QString currentPluginFileName;
    QStringList currentDriverList;
    QLibrary pluginLibrary;

    static timeval startTimeMS;
    static bool timeMSInitialized;

    void setLuaIOTypes(void);
    void setLuaAVRConstants(void);
    void initLua(void);
    void initAVRClock(void);
    void terminateAVRClock(void);
    void terminatePluginMainThread(void);
    void checkPluginThreadDelay(void);
    TIORegisterData getIORegister(EIORegisterTypes type) const;
    void setIORegister(EIORegisterTypes type, TIORegisterData data);

    // Callbacks for RP6 plugin
    static void IORegisterSetCB(EIORegisterTypes type, TIORegisterData value,
                                void *data);
    static TIORegisterData IORegisterGetCB(EIORegisterTypes type,
                                           void *data);
    static void enableISRsCB(bool e, void *data);

    // Timer callback
    static void timeOutCallback(CAVRTimer *timer, void *data);

    // Lua bindings
    static int luaAvrGetIORegister(lua_State *l);
    static int luaAvrSetIORegister(lua_State *l);
    static int luaAvrSetIORegisterIgnoreEqual(lua_State *l);
    static int luaAvrExecISR(lua_State *l);
    static int luaAvrSetTWIMSGHandler(lua_State *l);
    static int luaAvrSendTWIMSG(lua_State *l);
    static int luaClockSetTargetSpeed(lua_State *l);
    static int luaClockCreateTimer(lua_State *l);
    static int luaClockEnableTimer(lua_State *l);
    static int luaClockGetTimeMS(lua_State *l);
    static int luaTimerDestr(lua_State *l);
    static int luaTimerSetCompareValue(lua_State *l);
    static int luaTimerSetPrescaler(lua_State *l);
    static int luaTimerSetTimeOut(lua_State *l);
    static int luaTimerIsEnabled(lua_State *l);
    static int luaBitIsSet(lua_State *l);
    static int luaBitSet(lua_State *l);
    static int luaBitUnSet(lua_State *l);
    static int luaBitUnPack(lua_State *l);
    static int luaBitLower(lua_State *l);
    static int luaBitUpper(lua_State *l);
    static int luaBitAnd(lua_State *l);
    static int luaBitOr(lua_State *l);
    static int luaBitXor(lua_State *l);
    static int luaBitShiftLeft(lua_State *l);
    static int luaBitShiftRight(lua_State *l);

private slots:
    void handleLuaTWIMSG(const QString &msg, const QList<QVariant> &args);

public:
    explicit CSimulator(QObject *parent = 0);
    ~CSimulator(void);

    void startLua(const char *name);
    lua_State *getLuaState(void) const { return luaState; }
    void loadProjectFile(const QSettings &settings);
    void execISR(EISRTypes type);
    CAVRClock *getAVRClock(void) { return AVRClock; }
    QString getPluginLibraryName(void) { return currentPluginFileName; }
    bool initPlugin(void);
    void runPlugin(void);
    void stopPlugin(void);
#if USEATOMIC
    const QAtomicInt *getIORegisterArray(void) const { return IORegisterData; }
#else
    const TIORegisterData *getIORegisterArray(void) const { return IORegisterData; }
    QReadWriteLock &getIORegisterLock(void) { return IORegisterReadWriteLock; }
#endif

signals:
    void luaTWIMSGSend(const QString &msg, const QList<QVariant> &args);
};

#endif // SIMULATOR_H
