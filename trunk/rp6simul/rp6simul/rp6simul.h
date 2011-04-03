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

typedef void (*TCallPluginMainFunc)(void);

typedef void (*TGeneralIOSetCB)(EGeneralIOTypes, TGeneralIOData);
typedef TGeneralIOData (*TGeneralIOGetCB)(EGeneralIOTypes);
typedef void (*TSetGeneralIOCallbacks)(TGeneralIOSetCB, TGeneralIOGetCB);

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

class CAVRClock;
class CBaseIOHandler;
class CCallPluginMainThread;

class CRP6Simulator : public QMainWindow
{
    Q_OBJECT

    CAVRClock *AVRClock;
    QThread *AVRClockThread;
    CCallPluginMainThread *pluginMainThread;

    TGeneralIOData generalIOData[IO_END];
    CBaseIOHandler *IOHandlerArray[IO_END];
    QList<CBaseIOHandler *> IOHandlerList;

    typedef void (*TISR)(void);
    TISR ISRCacheArray[ISR_END];
    bool ISRFailedArray[ISR_END];

    static CRP6Simulator *instance;
    static QReadWriteLock generalIOReadWriteLock;
    static QMutex ISRExecMutex;

    void initAVRClock(void);
    void addIOHandler(CBaseIOHandler *handler);
    void initIOHandlers(void);
    void terminateAVRClock(void);
    void terminatePluginMainThread(void);
    void initPlugin(void);

    static void generalIOSetCB(EGeneralIOTypes type, TGeneralIOData data);
    static TGeneralIOData generalIOGetCB(EGeneralIOTypes type);

private slots:
    void runPlugin(void);

public:
    CRP6Simulator(QWidget *parent = 0);
    ~CRP6Simulator(void);

    TGeneralIOData getGeneralIO(EGeneralIOTypes type) const;
    void setGeneralIO(EGeneralIOTypes type, TGeneralIOData data);
    void execISR(EISRTypes type);
    CAVRClock *getAVRClock(void) { return AVRClock; }

    static CRP6Simulator *getInstance(void) { return instance; }
};

#endif // RP6SIMUL_H
