#ifndef AVRTIMER_H
#define AVRTIMER_H

#include "simulator.h"

#include <stdint.h>
#include <time.h>

#include <QDebug>
#include <QList>
#include <QObject>

class QTimer;

class CTicks
{
    enum { CYCLE_LENGTH = 1000000 };

    unsigned long cycles; // Completed cycles
    unsigned long ticks; // Ticks of current incomplete cycle

    void clamp(void)
    {
        while (ticks > CYCLE_LENGTH)
        {
            cycles++;
            ticks -= CYCLE_LENGTH;
        }
    }

    CTicks(unsigned long c, unsigned long t) : cycles(c), ticks(t) { }

    friend QDebug operator<<(QDebug dbg, const CTicks &ticks);

public:
    CTicks(void) : cycles(0), ticks(0) { }
    CTicks(unsigned long t) : cycles(0), ticks(t) { clamp(); }

    void reset(void) { cycles = ticks = 0; }
    void set(unsigned long t) { cycles = 0; ticks = t; clamp(); }
    unsigned long get(void) const { return (cycles * CYCLE_LENGTH) + ticks; }

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

    CTicks &operator-=(unsigned long t)
    {
        // Note: no overflow checking; other must be <

        if (ticks >= t)
            ticks -= t;
        else
        {
            t -= ticks;
            while (t > CYCLE_LENGTH)
            {
                cycles--;
                t -= CYCLE_LENGTH;
                Q_ASSERT(cycles);
            }
        }

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

    CTicks operator-(const CTicks &other) const
    {
        // Note: no overflow checking; other must be <
        unsigned long c = cycles - other.cycles, t;

        if (ticks >= other.ticks)
            t = ticks - other.ticks;
        else
        {
            c--;
            t = CYCLE_LENGTH - (other.ticks - ticks);
        }

        return CTicks(c, t);
    }

    bool operator ==(const CTicks &other) const
    {
        return ((other.cycles == cycles) && (other.ticks == ticks));
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

QDebug operator<<(QDebug dbg, const CTicks &ticks);


// Not really a 8 or 16 bit timer. Instead it uses a 32 bit uint that also
// takes care of any prescalers.
class CAVRTimer
{
public:
    typedef void (*TTimeOut)(CAVRTimer *, void *);

private:
    volatile bool enabled;
    TTimeOut timeOutCallback;
    void *timeOutData;
    CTicks nextTick;
    uint32_t compareValue, prescaler, trueCompareValue;

    void updateTrueCompareValue(void)
    {
        uint32_t v = compareValue * prescaler;
        if (nextTick.get() < trueCompareValue)
            nextTick.reset();
        else
            nextTick -= trueCompareValue;
        nextTick += v;
        trueCompareValue = v;
    }

public:
    CAVRTimer(TTimeOut t, void *d) : enabled(false), timeOutCallback(t),
        timeOutData(d), compareValue(0), prescaler(1), trueCompareValue(0) { }

    const CTicks &getNextTick(void) const { return nextTick; }
    CTicks &getRefNextTick(void) { return nextTick; }
    void setCompareValue(uint32_t c)
    { compareValue = c; updateTrueCompareValue(); }
    void setPrescaler(uint32_t p) { prescaler = p; updateTrueCompareValue(); }
    uint32_t getTrueCompareValue(void) const { return trueCompareValue; }
    void timeOut(void)
    { if (timeOutCallback) (*timeOutCallback)(this, timeOutData); }
    bool isEnabled(void) const { return enabled; }
    void setEnabled(bool e) { enabled = e; }
};

class CAVRClock: public QObject
{
public:
    typedef QList<CAVRTimer *> TTimerList;

private:
    Q_OBJECT

    unsigned long targetClockSpeed;
    int maxTimeOutsCycle, sleepTime; // sleep time in microseconds
    TTimerList timerList;
    CTicks currentTicks, remainingTicks;
    QTimer *clockTimer;
    timespec lastClockTime;
    bool initClockTime;

    // Timer stats: used for debugging and calculating effective speed
    unsigned long totalDeltaTime, runsPerSec, timeOutsPerSec;
    unsigned long totalTimeOutTime;
    CTicks ticksPerSec;

    CAVRTimer *getClosestTimer(void);

private slots:
    void run(void);

public:
    CAVRClock(void);
    ~CAVRClock(void);

    void setTargetClockSpeed(unsigned long t) { targetClockSpeed = t; }
    void setCPUUsage(ESimulatorCPUUsage usage);
    CAVRTimer *createTimer(CAVRTimer::TTimeOut t, void *d);
    void enableTimer(CAVRTimer *timer, bool e);
    void removeTimer(CAVRTimer *timer);
    TTimerList &getTimers(void) { return timerList; }
    void start(void);
    bool isActive(void) const;

public slots:
    void stop(void);

signals:
    void startTimer(void);
    void stopTimer(void);
    void clockSpeed(unsigned long);
};

#endif // AVRTIMER_H
