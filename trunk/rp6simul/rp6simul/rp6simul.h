#ifndef RP6SIMUL_H
#define RP6SIMUL_H

#include "shared.h"

#include <stdint.h>

#include <QtGui/QMainWindow>

#include <QList>
#include <QMap>
#include <QMutex>
#include <QReadWriteLock>
#include <QString>

#include <lua.hpp>

typedef void (*TCallPluginMainFunc)(void);

typedef void (*TIORegisterSetCB)(EIORegisterTypes, TIORegisterData);
typedef TIORegisterData (*TIORegisterGetCB)(EIORegisterTypes);
typedef void (*TSetIORegisterCallbacks)(TIORegisterSetCB, TIORegisterGetCB);

enum EISRTypes
{
    // UART
    ISR_USART_RXC_vect=0,

    // Timers
    ISR_TIMER0_COMP_vect,
    ISR_TIMER1_COMPA_vect,
    ISR_TIMER2_COMP_vect,

    ISR_END
};

class QPlainTextEdit;

class CAVRClock;
class CBaseIOHandler;
class CCallPluginMainThread;

class CRP6Simulator : public QMainWindow
{
    Q_OBJECT

    enum ELogType { LOG_LOG, LOG_DEBUG, LOG_WARNING, LOG_ERROR };

    CAVRClock *AVRClock;
    QThread *AVRClockThread;
    CCallPluginMainThread *pluginMainThread;

    TIORegisterData IORegisterData[IO_END];
    CBaseIOHandler *IOHandlerArray[IO_END];
    QList<CBaseIOHandler *> IOHandlerList;

    typedef void (*TISR)(void);
    TISR ISRCacheArray[ISR_END];
    bool ISRFailedArray[ISR_END];

    QPlainTextEdit *logWidget;

    static CRP6Simulator *instance;
    static QReadWriteLock IORegisterReadWriteLock;
    static QMutex ISRExecMutex;

    void initAVRClock(void);
    void addIOHandler(CBaseIOHandler *handler);
    void initIOHandlers(void);
    void setLuaIOTypes(void);
    void setLuaAVRConstants(void);
    void initLua(void);
    void terminateAVRClock(void);
    void terminatePluginMainThread(void);
    void initPlugin(void);
    void appendLogOutput(ELogType type, const QString &text);

    static void IORegisterSetCB(EIORegisterTypes type, TIORegisterData data);
    static TIORegisterData IORegisterGetCB(EIORegisterTypes type);

    // Lua bindings
    static int luaAvrGetIORegister(lua_State *l);
    static int luaAvrSetIORegister(lua_State *l);
    static int luaClockCreateTimer(lua_State *l);
    static int luaClockEnableTimer(lua_State *l);
    static int luaTimerDestr(lua_State *l);
    static int luaTimerSetCompareValue(lua_State *l);
    static int luaTimerSetPrescaler(lua_State *l);
    static int luaTimerSetTimeOut(lua_State *l);
    static int luaTimerIsEnabled(lua_State *l);
    static int luaBitIsSet(lua_State *l);
    static int luaBitSet(lua_State *l);
    static int luaBitUnpack(lua_State *l);
    static int luaAppendLogOutput(lua_State *l);

private slots:
    void runPlugin(void);

public:
    CRP6Simulator(QWidget *parent = 0);
    ~CRP6Simulator(void);

    TIORegisterData getIORegister(EIORegisterTypes type) const;
    void setIORegister(EIORegisterTypes type, TIORegisterData data);
    void execISR(EISRTypes type);
    CAVRClock *getAVRClock(void) { return AVRClock; }

    static CRP6Simulator *getInstance(void) { return instance; }
};

#endif // RP6SIMUL_H
