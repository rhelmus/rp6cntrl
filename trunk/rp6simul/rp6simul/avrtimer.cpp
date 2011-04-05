#include "avrtimer.h"
#include "lua.h"

#include <QDebug>
#include <QTimer>

namespace {

unsigned long getusecs(const timespec &start, const timespec &end)
{
    return ((end.tv_sec-start.tv_sec) * 1000000) +
            ((end.tv_nsec-start.tv_nsec) / 1000);
}

}

CAVRTimer::~CAVRTimer()
{
    if (timeOutType == TIMEOUT_LUA)
        luaL_unref(NLua::luaInterface, LUA_REGISTRYINDEX, luaTimeOutRef);
}

void CAVRTimer::timeOutLua()
{
    lua_rawgeti(NLua::luaInterface, LUA_REGISTRYINDEX, luaTimeOutRef);
    lua_call(NLua::luaInterface, 0, 0); // UNDONE: error handling
}

void CAVRTimer::setTimeOutLua()
{
    // Lua function should be at top of stack
    timeOutType = TIMEOUT_LUA;
    luaTimeOutRef = luaL_ref(NLua::luaInterface, LUA_REGISTRYINDEX);
}


CAVRClock::CAVRClock(void) : initClockTime(true)
{
    clockTimer = new QTimer(this);
    clockTimer->setInterval(0);
    connect(clockTimer, SIGNAL(timeout()), this, SLOT(run()));
    connect(this, SIGNAL(startTimer()), clockTimer, SLOT(start()));
    connect(this, SIGNAL(stopTimer()), clockTimer, SLOT(stop()));
}

CAVRClock::~CAVRClock()
{
    // UNDONE: Needed?
//    foreach(CAVRTimer *timer, timerList)
//        delete timer;
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
            timer->timeOut();
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

CAVRTimer *CAVRClock::createTimer()
{
    CAVRTimer *ret = new CAVRTimer;
    timerList << ret;
    return ret;
}

void CAVRClock::enableTimer(CAVRTimer *timer, bool e)
{
    if (e != timer->isEnabled())
    {
        if (e) // Init timer?
            timer->getRefNextTick() = currentTicks + timer->getTrueCompareValue();
        timer->setEnabled(e);
    }
}

void CAVRClock::reset()
{
    currentTicks.reset();
    // UNDONE: Needed?
//    foreach(CAVRTimer *timer, timerList)
//        timer->setEnabled(false);
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
