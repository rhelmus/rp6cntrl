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

    ISR_END
};

class QLCDNumber;
class QPlainTextEdit;
class QTableWidget;

class CAVRClock;
class CCallPluginMainThread;
class CProjectWizard;

class CRP6Simulator : public QMainWindow
{
    Q_OBJECT

    enum ELogType { LOG_LOG, LOG_WARNING, LOG_ERROR };

    CAVRClock *AVRClock;
    QThread *AVRClockThread;

    CCallPluginMainThread *pluginMainThread;
    timespec lastPluginDelay;

    TIORegisterData IORegisterData[IO_END];
    mutable QReadWriteLock IORegisterReadWriteLock;

    bool ISRsEnabled;
    typedef void (*TISR)(void);
    TISR ISRCacheArray[ISR_END];
    bool ISRFailedArray[ISR_END];
    QMutex ISRExecMutex;

    CProjectWizard *projectWizard;

    QPlainTextEdit *logWidget;
    QPlainTextEdit *serialOutputWidget;
    QLCDNumber *clockDisplay;
    QTableWidget *IORegisterTableWidget;

    QString serialTextBuffer;
    QMutex serialBufferMutex;

    QTimer *pluginUpdateUITimer;

    static CRP6Simulator *instance;

    void createMenus(void);
    void createToolbars(void);
    QWidget *createMainWidget(void);
    QWidget *createLogWidgets(void);
    QDockWidget *createStatusDock(void);
    QDockWidget *createRegisterDock(void);

    void initAVRClock(void);
    void setLuaIOTypes(void);
    void setLuaAVRConstants(void);
    void initLua(void);
    void terminateAVRClock(void);
    void terminatePluginMainThread(void);
    void openProjectFile(const QString &file);
    void closeProject(void);
    void initPlugin(void);
    void checkPluginThreadDelay(void);
    QString getLogOutput(ELogType type, QString text) const;
    void appendLogOutput(ELogType type, const QString &text);

    // Callbacks for RP6 plugin
    static void IORegisterSetCB(EIORegisterTypes type, TIORegisterData data);
    static TIORegisterData IORegisterGetCB(EIORegisterTypes type);
    static void enableISRsCB(bool e);

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
    static int luaBitUnSet(lua_State *l);
    static int luaBitUnPack(lua_State *l);
    static int luaBitAnd(lua_State *l);
    static int luaAppendLogOutput(lua_State *l);
    static int luaAppendSerialOutput(lua_State *l);

private slots:
    void updateClockDisplay(unsigned long hz);
    void timedUpdate(void);
    void newProject(void);
    void openProject(void);
    void runPlugin(void);

public:
    CRP6Simulator(QWidget *parent = 0);
    ~CRP6Simulator(void);

    TIORegisterData getIORegister(EIORegisterTypes type) const;
    void setIORegister(EIORegisterTypes type, TIORegisterData data);
    void execISR(EISRTypes type);
    CAVRClock *getAVRClock(void) { return AVRClock; }

    static CRP6Simulator *getInstance(void) { return instance; }

signals:
    void logTextReady(const QString &text);
};

inline unsigned long getUSDiff(const timespec &start, const timespec &end)
{
    return ((end.tv_sec-start.tv_sec) * 1000000) +
            ((end.tv_nsec-start.tv_nsec) / 1000);
}


#endif // RP6SIMUL_H
