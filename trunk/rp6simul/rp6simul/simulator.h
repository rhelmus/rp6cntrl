#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "clock.h"
#include "lua.h"
#include "shared.h"

#include <time.h>

#include <QLibrary>
#include <QMutex>
#include <QObject>
#include <QReadWriteLock>
#include <QStringList>

// UNDONE: Move to shared.h?
typedef void (*TCallPluginMainFunc)(void);
typedef void (*TIORegisterSetCB)(EIORegisterTypes, TIORegisterData);
typedef TIORegisterData (*TIORegisterGetCB)(EIORegisterTypes);
typedef void (*TEnableISRsCB)(bool);
typedef void (*TSetPluginCallbacks)(TIORegisterSetCB, TIORegisterGetCB,
                                    TEnableISRsCB);

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
    ISR_INT2_vect,

    ISR_END
};

class QSettings;

class CAVRClock;
class CCallPluginMainThread;

class CSimulator : public QObject
{
    Q_OBJECT

    CAVRClock *AVRClock;
    QThread *AVRClockThread;

    CCallPluginMainThread *pluginMainThread;
    timespec lastPluginDelay;
    volatile bool quitPlugin; // UNDONE: Volatile OK?

    TIORegisterData IORegisterData[IO_END];
    mutable QReadWriteLock IORegisterReadWriteLock;

    bool ISRsEnabled;
    typedef void (*TISR)(void);
    TISR ISRCacheArray[ISR_END];
    bool ISRFailedArray[ISR_END];
    QMutex ISRExecMutex;

    QString currentPluginFileName;
    QStringList currentDriverList;
    QLibrary RP6Plugin;

    typedef QHash<QString, QVariant> TRobotPropertyFields;
    typedef QHash<QString, TRobotPropertyFields> TRobotProperties;
    TRobotProperties robotProperties;

    static CSimulator *instance;

    void initAVRClock(void);
    void setLuaIOTypes(void);
    void setLuaAVRConstants(void);
    void loadRobotProperties(void);
    void terminateAVRClock(void);
    void terminatePluginMainThread(void);
    bool initPlugin(void);
    void checkPluginThreadDelay(void);
    TIORegisterData getIORegister(EIORegisterTypes type) const;
    void setIORegister(EIORegisterTypes type, TIORegisterData data);

    // Callbacks for RP6 plugin
    static void IORegisterSetCB(EIORegisterTypes type, TIORegisterData data);
    static TIORegisterData IORegisterGetCB(EIORegisterTypes type);
    static void enableISRsCB(bool e);

    // Lua bindings
    static int luaAvrGetIORegister(lua_State *l);
    static int luaAvrSetIORegister(lua_State *l);
    static int luaAvrExecISR(lua_State *l);
    static int luaClockCreateTimer(lua_State *l);
    static int luaClockEnableTimer(lua_State *l);
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
    static int luaBitXor(lua_State *l);
    static int luaBitShiftLeft(lua_State *l);
    static int luaBitShiftRight(lua_State *l);

public:
    explicit CSimulator(QObject *parent = 0);
    ~CSimulator(void);

    void initLua(void);
    bool loadProjectFile(const QSettings &settings);
    void execISR(EISRTypes type);
    CAVRClock *getAVRClock(void) { return AVRClock; }
    bool runPlugin(void);
    void stopPlugin(void);
    const TIORegisterData *getIORegisterArray(void) const { return IORegisterData; }
    QReadWriteLock &getIORegisterLock(void) { return IORegisterReadWriteLock; }
    QVariant getRobotProperty(const QString &prop, const QString &field) const
    { return robotProperties[prop][field]; }

    static CSimulator *getInstance(void) { return instance; }

};

#endif // SIMULATOR_H
