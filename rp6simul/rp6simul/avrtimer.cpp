#include "avrtimer.h"

#include <QDebug>
#include <QTimer>

namespace {

unsigned long getusecs(const timespec &start, const timespec &end)
{
    return ((end.tv_sec-start.tv_sec) * 1000000) +
            ((end.tv_nsec-start.tv_nsec) / 1000);
}

}

CAVRClock::CAVRClock(void) : initClockTime(true)
{
    // UNDONE: Support TIMER1B?
    timerArray[TIMER_0] = new CAVRTimer(ISR_TIMER0_COMP_vect);
    timerArray[TIMER_1A] = new CAVRTimer(ISR_TIMER1_COMPA_vect);
    timerArray[TIMER_2] = new CAVRTimer(ISR_TIMER2_COMP_vect);

    clockTimer = new QTimer(this);
    clockTimer->setInterval(0);
    connect(clockTimer, SIGNAL(timeout()), this, SLOT(run()));
    connect(this, SIGNAL(startTimer()), clockTimer, SLOT(start()));
    connect(this, SIGNAL(stopTimer()), clockTimer, SLOT(stop()));
}

CAVRClock::~CAVRClock()
{
    for (int i=0; i<TIMER_END; ++i)
        delete timerArray[i];
}

CAVRTimer *CAVRClock::getClosestTimer()
{
    CAVRTimer *closest = 0;

    for (int i=0; i<TIMER_END; ++i)
    {
        if (!timerArray[i]->isEnabled())
            continue;

        if (!closest || (timerArray[i]->getNextTick() < closest->getNextTick()))
            closest = timerArray[i];
    }

    return closest;
}

#include <QThread>

void CAVRClock::run()
{
    static timespec curtime;
    clock_gettime(CLOCK_MONOTONIC, &curtime);

    if (initClockTime)
    {
        initClockTime = false;
        lastClockTime = curtime;
        return;
    }

    static unsigned long delta_total = 0, delta_count = 0;

    const unsigned long delta = getusecs(lastClockTime, curtime);
    const CTicks newticks(RP6_CLOCK / 1000000 * delta);
    const CTicks finalticks = currentTicks + newticks;

    delta_total += delta;
    delta_count++;

    if (delta_total >= 1000000)
    {
        qDebug() << "AVG delta:" << (delta_total / delta_count);
        delta_total = delta_count = 0;
    }

    static CAVRTimer *timer;
    while (true)
    {
        timer = getClosestTimer();
        if (!timer)
            break;

        if (finalticks > timer->getNextTick())
        {
            currentTicks = timer->getNextTick();
            timer->getRefNextTick() += timer->getTrueCompareValue();
            timer->execISR();
        }
        else
            break;
    }

    currentTicks = finalticks;
    lastClockTime = curtime;

    // Relieve CPU a bit
    timespec ts = { 0, 1000 };
    nanosleep(&ts, 0);
}

void CAVRClock::enableTimer(EAVRTimers timer, bool e)
{
    CAVRTimer *t = timerArray[timer];
    if (e != t->isEnabled())
    {
        if (e) // Init timer?
            t->getRefNextTick() = currentTicks + t->getTrueCompareValue();
        t->setEnabled(e);
    }
}

void CAVRClock::reset()
{
    currentTicks.reset();
    for (int i=0; i<TIMER_END; ++i)
        timerArray[i]->setEnabled(false);
    initClockTime = true;
}

void CAVRClock::start()
{
    emit startTimer();
}

void CAVRClock::stop()
{
    emit stopTimer();
}
