#include "avrtimer.h"
#include "lua.h"

#include <QDebug>
#include <QTimer>


QDebug operator<<(QDebug dbg, const CTicks &ticks)
{
    dbg.nospace() << "CTicks(" << ((ticks.cycles * RP6_CLOCK) + ticks.ticks) << ")";
    return dbg.maybeSpace();
}


CAVRTimer::~CAVRTimer()
{
    if (timeOutType == TIMEOUT_LUA)
        luaL_unref(NLua::luaInterface, LUA_REGISTRYINDEX, luaTimeOutRef);
}

void CAVRTimer::timeOutLua()
{
    NLua::CLuaLocker lualocker;
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
    static unsigned long timeout_total = 0, timeout_count = 0;
    static CTicks tickspersec = 0;

    const unsigned long delta = getUSDiff(lastClockTime, curtime);

    const CTicks newticks(remainingTicks + (RP6_CLOCK / 1000000 * delta));
    const CTicks finalticks = currentTicks + newticks;
    const CTicks curticks(currentTicks);

    delta_total += delta;
    delta_count++;

    static CAVRTimer *timer;
    static int timeouts;

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

    tickspersec += (currentTicks - curticks);
    remainingTicks = finalticks - currentTicks;

    timeout_total += timeouts;
    timeout_count++;

    if (delta_total >= 1000000)
    {
        qDebug() << "delta_total:" << delta_total;
        qDebug() << "AVG delta:" << (delta_total / delta_count);
        qDebug() << "AVG timeout:" << (timeout_total / timeout_count);
        qDebug() << "Frequency (ticks/s):" << tickspersec;
        delta_total = delta_count = 0;
        timeout_total = timeout_count = 0;

        // UNDONE: Move out of debug code
        emit clockSpeed(tickspersec.get());
        tickspersec.reset();
    }

    // Relieve CPU a bit
    // UNDONE: Make optional (same reasoning as above: get more accurate MHz for fast timers)
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

void CAVRClock::removeTimer(CAVRTimer *timer)
{
    qDebug() << "Removed timer from clock";
    timerList.removeOne(timer);
}

void CAVRClock::reset()
{
    currentTicks.reset();
    remainingTicks.reset();
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
