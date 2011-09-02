#include "avrtimer.h"
#include "lua.h"
#include "utils.h"

#include <unistd.h>

#include <QDebug>
#include <QTimer>


QDebug operator<<(QDebug dbg, const CTicks &ticks)
{
    dbg.nospace() << "CTicks(" << ((ticks.cycles * CTicks::CYCLE_LENGTH) + ticks.ticks) << ")";
    return dbg.maybeSpace();
}


CAVRClock::CAVRClock(void) : targetClockSpeed(0), initClockTime(true),
    totalDeltaTime(0), runsPerSec(0), timeOutsPerSec(0), totalTimeOutTime(0)
{
    clockTimer = new QTimer(this);
    clockTimer->setInterval(0);
    connect(clockTimer, SIGNAL(timeout()), this, SLOT(run()));
    connect(this, SIGNAL(startTimer()), clockTimer, SLOT(start()));
    connect(this, SIGNAL(stopTimer()), clockTimer, SLOT(stop()));
}

CAVRClock::~CAVRClock()
{
}

CAVRTimer *CAVRClock::getClosestTimer()
{
    CAVRTimer *closest = 0;

    foreach(CAVRTimer *timer, timerList)
    {
        if (!timer->isEnabled())
            continue;

        if (!closest || (timer->getNextTick() < closest->getNextTick()))
            closest = timer;
    }

    return closest;
}

void CAVRClock::run()
{
    Q_ASSERT(targetClockSpeed != 0);

    timespec curtime;
    clock_gettime(CLOCK_MONOTONIC, &curtime);

    if (initClockTime)
    {
        initClockTime = false;
        lastClockTime = curtime;
        return;
    }

    const unsigned long delta = getUSDiff(lastClockTime, curtime);

    const CTicks newticks(remainingTicks + (targetClockSpeed / 1000000 * delta));
    const CTicks finalticks = currentTicks + newticks;
    const CTicks curticks(currentTicks);

    totalDeltaTime += delta;
    runsPerSec++;

    CAVRTimer *timer;
    int timeouts;

    currentTicks = finalticks; // May be less, see below
    lastClockTime = curtime;

    timeouts = 0;

    // UNDONE: Make this an option; ie higher value gives 'faster timer' (more correct MHz), but hammers CPU more.
    while (timeouts < 15000)
    {
        timer = getClosestTimer();
        if (!timer)
            break;

        if (finalticks > timer->getNextTick())
        {
            currentTicks = timer->getNextTick();
            timer->getRefNextTick() += timer->getTrueCompareValue();
            timer->timeOut();
            timeouts++;
        }
        else
        {
            currentTicks = finalticks;
            break;
        }
    }

    ticksPerSec += (currentTicks - curticks);
    remainingTicks = finalticks - currentTicks;

    timeOutsPerSec += timeouts;

    clock_gettime(CLOCK_MONOTONIC, &curtime);
    totalTimeOutTime += getUSDiff(lastClockTime, curtime);

    if (totalDeltaTime >= 1000000)
    {
        qDebug() << "totalDeltaTime:" << totalDeltaTime << (void*)this;
        qDebug() << "runsPerSec:" << runsPerSec << (void*)this;
        qDebug() << "AVG delta:" << (totalDeltaTime / runsPerSec) << (void*)this;
        qDebug() << "AVG timeout:" << (timeOutsPerSec / runsPerSec) << (void*)this;
        qDebug() << "Frequency (ticks/s):" << ticksPerSec << (void*)this;
        qDebug() << "avg timeout time:" << totalTimeOutTime / runsPerSec << (void*)this;
        totalDeltaTime = runsPerSec = 0;
        timeOutsPerSec = 0;
        totalTimeOutTime = 0;

        // UNDONE: Move out of debug code
        emit clockSpeed(ticksPerSec.get());
        ticksPerSec.reset();
    }

    // Relieve CPU a bit
    // UNDONE: Make optional (same reasoning as above: get more accurate MHz for fast timers)

#ifdef Q_OS_WIN32
    usleep(1);
#else
    timespec ts = { 0, 1000 };
    nanosleep(&ts, 0);
#endif
}

CAVRTimer *CAVRClock::createTimer(CAVRTimer::TTimeOut t, void *d)
{
    CAVRTimer *ret = new CAVRTimer(t, d);
    timerList << ret;
    qDebug() << "Added new timer: " << (void *)ret <<
                "(" << timerList.count() << "registered)";
    return ret;
}

void CAVRClock::enableTimer(CAVRTimer *timer, bool e)
{
    if (e != timer->isEnabled())
    {
        if (e) // Init timer?
        {
            // UNDONE: Always do this when being enabled?
            timer->getRefNextTick() =
                    currentTicks + timer->getTrueCompareValue();
#if 0
            const CTicks maxnexttick(currentTicks + timer->getTrueCompareValue());
            if (maxnexttick < timer->getNextTick())
                timer->getRefNextTick() = maxnexttick;
#endif
        }
        timer->setEnabled(e);
    }
}

void CAVRClock::removeTimer(CAVRTimer *timer)
{
    qDebug() << "Removed timer from clock:" << (void *)timer;
    timerList.removeOne(timer);
}

void CAVRClock::start()
{
    currentTicks.reset();
    remainingTicks.reset();
    initClockTime = true;
    emit startTimer();
}

bool CAVRClock::isActive() const
{
    return clockTimer->isActive();
}

void CAVRClock::stop()
{
    emit stopTimer();
}
