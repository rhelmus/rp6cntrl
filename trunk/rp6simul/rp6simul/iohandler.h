#ifndef IOHANDLER_H
#define IOHANDLER_H

#include "rp6simul.h"
#include "shared.h"

#include <stdint.h>

#include <QList>
#include <QObject>
#include <QString>

class CAVRTimer;

// UNDONE: Call/need reset?

class CBaseIOHandler: public QObject
{
public:
    explicit CBaseIOHandler(QObject *parent) : QObject(parent) { }

    virtual void handleIOData(EGeneralIOTypes, TGeneralIOData) = 0;
    virtual void reset(void) { }
    virtual void registerHandler(CBaseIOHandler **array) = 0;
    virtual void initPlugin(void) { }
};

class CUARTHandler: public CBaseIOHandler
{
    Q_OBJECT

    QString outLine;

    void dumpLine(void); // Prints and clears line buffer

private slots:
    void writeText(void); // UNDONE: Remove (debug)

public:
    explicit CUARTHandler(QObject *parent) : CBaseIOHandler(parent) { }

    virtual void handleIOData(EGeneralIOTypes type, TGeneralIOData data);
    virtual void reset(void);
    virtual void registerHandler(CBaseIOHandler **array);
    virtual void initPlugin(void);
};

class CTimer0Handler: public CBaseIOHandler
{
public:
    explicit CTimer0Handler(QObject *parent) : CBaseIOHandler(parent) { }

    virtual void handleIOData(EGeneralIOTypes type, TGeneralIOData data);
    virtual void registerHandler(CBaseIOHandler **array);
    virtual void initPlugin(void);
};

class CTimer1Handler: public CBaseIOHandler
{
    TGeneralIOData channelA, channelB;
    int prescaler;
    TGeneralIOData compareValue;

    bool PWMEnabled(void) const;
    int getPrescaler(void) const;
    void updateTimerEnabled(void);

public:
    explicit CTimer1Handler(QObject *parent) : CBaseIOHandler(parent) { }

    virtual void handleIOData(EGeneralIOTypes type, TGeneralIOData data);
    virtual void registerHandler(CBaseIOHandler **array);
    virtual void initPlugin(void);
};

class CTimer2Handler: public CBaseIOHandler
{
public:
    explicit CTimer2Handler(QObject *parent) : CBaseIOHandler(parent) { }

    virtual void handleIOData(EGeneralIOTypes type, TGeneralIOData data);
    virtual void registerHandler(CBaseIOHandler **array);
    virtual void initPlugin(void);
};

class CTimerMaskHandler: public CBaseIOHandler
{
public:
    explicit CTimerMaskHandler(QObject *parent) : CBaseIOHandler(parent) { }

    virtual void handleIOData(EGeneralIOTypes type, TGeneralIOData data);
    virtual void registerHandler(CBaseIOHandler **array);
};



#endif // IOHANDLER_H
