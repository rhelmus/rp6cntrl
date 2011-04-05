#ifndef AVRTIMER_H
#define AVRTIMER_H

#include "rp6simul.h"

#include <stdint.h>
#include <time.h>

#include <QList>
#include <QObject>

class QTimer;

// RP6 is running at 8 mhz. UNDONE: Make this configurable
enum { RP6_CLOCK = 8000000 };

class CTicks
{
    unsigned long cycles; // Completed 1 second cycles
    unsigned long ticks; // Ticks of current incomplete cycle

    void clamp(void)
    {
        while (ticks > RP6_CLOCK)
        {
            cycles++;
            ticks -= RP6_CLOCK;
        }
    }

    CTicks(unsigned long c, unsigned long t) : cycles(c), ticks(t) { }

public:
    CTicks(void) : cycles(0), ticks(0) { }
    CTicks(unsigned long t) : cycles(0), ticks(t) { clamp(); }

    void reset(void) { cycles = ticks = 0; }
    void set(unsigned long t) { cycles = 0; ticks = t; clamp(); }

    CTicks &operator+=(const CTicks &other)
    {
        cycles += other.cycles; ticks += other.ticks;
        clamp();
        return *this;
    }

    CTicks &operator+=(unsigned long t)
    {
        ticks += t;
        clamp();
        return *this;
    }

    CTicks operator+(const CTicks &other) const
    {
        CTicks ret(cycles + other.cycles, ticks + other.ticks);
        ret.clamp();
        return ret;
    }

    CTicks operator+(unsigned long t) const
    {
        CTicks ret(cycles, ticks + t);
        ret.clamp();
        return ret;
    }

    bool operator<(const CTicks &other) const
    {
        if (cycles < other.cycles)
            return true;
        if (other.cycles == cycles)
            return ticks < other.ticks;
        return false;
    }

    bool operator<=(const CTicks &other) const
    {
        if (cycles < other.cycles)
            return true;
        if (other.cycles == cycles)
            return ticks <= other.ticks;
        return false;
    }

    bool operator>(const CTicks &other) const
    {
        if (cycles > other.cycles)
            return true;
        if (other.cycles == cycles)
            return ticks > other.ticks;
        return false;
    }

    bool operator>=(const CTicks &other) const
    {
        if (cycles > other.cycles)
            return true;
        if (other.cycles == cycles)
            return ticks >= other.ticks;
        return false;
    }
};

// Not really a 8 or 16 bit timer. Instead it uses a 32 bit uint that also
// takes care of any prescalers.
// The timer either calls an ISR or lua function
class CAVRTimer
{
    bool enabled;
    CTicks nextTick;
    EISRTypes ISRTimeOut; // ISR to call
    int luaTimeOutRef;
    enum { TIMEOUT_ISR, TIMEOUT_LUA, TIMEOUT_NONE } timeOutType;
    uint32_t compareValue, prescaler, trueCompareValue;

    void updateTrueCompareValue(void)
    { trueCompareValue = compareValue * prescaler; }
    void timeOutLua(void);

public:
    CAVRTimer(void) : enabled(false), timeOutType(TIMEOUT_NONE),
        compareValue(0), prescaler(1), trueCompareValue(0) { }
    ~CAVRTimer(void);

    const CTicks &getNextTick(void) const { return nextTick; }
    CTicks &getRefNextTick(void) { return nextTick; }
    void setTimeOutISR(EISRTypes isr)
    { timeOutType = TIMEOUT_ISR; ISRTimeOut = isr; }
    void setTimeOutLua(void);
    void setCompareValue(uint32_t c)
    { compareValue = c; updateTrueCompareValue(); }
    void setPrescaler(uint32_t p) { prescaler = p; updateTrueCompareValue(); }
    uint32_t getTrueCompareValue(void) const { return trueCompareValue; }
    void timeOut(void)
    {
        if (timeOutType == TIMEOUT_ISR)
            CRP6Simulator::getInstance()->execISR(ISRTimeOut);
        else if (timeOutType == TIMEOUT_LUA)
            timeOutLua();
    }
    bool isEnabled(void) const { return enabled; }
    void setEnabled(bool e) { enabled = e; }
};

class CAVRClock: public QObject
{
    Q_OBJECT

    typedef QList<CAVRTimer *> TTimerList;
    TTimerList timerList;
    CTicks currentTicks;
    QTimer *clockTimer;
    timespec lastClockTime;
    bool initClockTime;

    CAVRTimer *getClosestTimer(void);

private slots:
    void run(void);

public:
    CAVRClock(void);
    ~CAVRClock(void);

    CAVRTimer *createTimer(void);
    void enableTimer(CAVRTimer *timer, bool e);
    void reset(void);

public slots:
    void start(void);
    void stop(void);

signals:
    void startTimer(void);
    void stopTimer(void);
};

#endif // AVRTIMER_H
