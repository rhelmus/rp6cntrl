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

void CAVRClock::run()
{
    timespec curtime;
    clock_gettime(CLOCK_REALTIME, &curtime);

    if (initClockTime)
    {
        initClockTime = false;
        lastClockTime = curtime;
        return;
    }

    const unsigned long delta = getusecs(lastClockTime, curtime);
    const CTicks newticks(RP6_CLOCK / 1000000 * delta);
    const CTicks finalticks = currentTicks + newticks;

//    qDebug() << "delta:" << delta << "newticks:" << RP6_CLOCK / 1000000 * delta;

    while (true)
    {
        CAVRTimer *timer = getClosestTimer();
        if (!timer)
            break;

        if (finalticks > timer->getNextTick())
        {
            currentTicks = timer->getNextTick();
            timer->getRefNextTick() += timer->getCompareValue();
            timer->execISR();
        }
        else
            break;
    }

    currentTicks = finalticks;
    lastClockTime = curtime;
}

void CAVRClock::enableTimer(EAVRTimers timer, bool e)
{
    CAVRTimer *t = timerArray[timer];
    if (e != t->isEnabled())
    {
        if (e) // Init timer?
            t->getRefNextTick() = currentTicks + t->getCompareValue();
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
//    clockTimer->start();
}

void CAVRClock::stop()
{
    emit stopTimer();
//    clockTimer->stop();
}
