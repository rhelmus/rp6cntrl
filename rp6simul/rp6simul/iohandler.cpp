#include "iohandler.h"
#include "avrtimer.h"

#include <QDebug>

#include <QTimer>

namespace {

inline bool bitSet(TGeneralIOData data, TGeneralIOData bit)
{
    return (data & (1<<bit));
}

CAVRTimer *getAVRTimer(CAVRClock::EAVRTimers t)
{
    return CRP6Simulator::getInstance()->getAVRClock()->getTimer(t);
}

}

void CUARTHandler::dumpLine()
{
    qDebug() << "UART line:" << outLine;
    outLine.clear();
}

void CUARTHandler::writeText()
{
    const char *text = "Hello AVR!\n";

    for (const char *c = text; c && *c; ++c)
    {
        CRP6Simulator::getInstance()->setGeneralIO(IO_UDR, *c);
        CRP6Simulator::getInstance()->execISR(ISR_USART_RXC_vect);
    }
}

void CUARTHandler::handleIOData(EGeneralIOTypes type, TGeneralIOData data)
{
    switch (type)
    {
    case IO_UCSRA: qDebug() << "UCSRA changed:" << data; break;
    case IO_UDR:
    {
        if (data == '\n')
            dumpLine();
        else
            outLine += data;
    }
    default: break;
    }
}

void CUARTHandler::reset()
{
    dumpLine();
}

void CUARTHandler::registerHandler(CBaseIOHandler **array)
{
    array[IO_UCSRA] = this;
    array[IO_UDR] = this;

    // UNDONE: Remove (debug)
//    QTimer *timer = new QTimer(this);
//    connect(timer, SIGNAL(timeout()), this, SLOT(writeText()));
//    timer->start(1000);
}

void CUARTHandler::initPlugin()
{
    // UNDONE (define, variable or something)
    // UNDONE: also this flag should be set even when it's changed elsewhere (always 'ready')
    CRP6Simulator::getInstance()->setGeneralIO(IO_UCSRA, 1<<5);
}


void CTimer0Handler::handleIOData(EGeneralIOTypes type, TGeneralIOData data)
{
    CAVRTimer *timer = getAVRTimer(CAVRClock::TIMER_0);
    if (type == IO_TCCR0)
    {
        // UNDONE: Neater error handling
        // UNDONE: More config options handled

        if ((data & (1<<WGM00)) || !(data & (1<<WGM01)))
            qDebug() << "Unsupported timer0 mode set!";

        if ((data & (1<<COM00)) || (data & (1<<COM01)))
            qDebug() << "Unsupported COM set for timer0!";

        // Prescaler
        if (!bitSet(data, CS00) && !bitSet(data, CS01) && !bitSet(data, CS02))
            timer->setPrescaler(0);
        else if (bitSet(data, CS00) && !bitSet(data, CS01) &&
                 !bitSet(data, CS02))
            timer->setPrescaler(1);
        else if (bitSet(data, CS00) && !bitSet(data, CS01) &&
                 !bitSet(data, CS02))
            timer->setPrescaler(1);
        else if (!bitSet(data, CS00) && bitSet(data, CS01) &&
                 !bitSet(data, CS02))
            timer->setPrescaler(8);
        else if (!bitSet(data, CS00) && !bitSet(data, CS01) &&
                 bitSet(data, CS02))
            timer->setPrescaler(256);
        else if (bitSet(data, CS00) && bitSet(data, CS01) &&
                 !bitSet(data, CS02))
            timer->setPrescaler(64);
        else if (bitSet(data, CS00) && !bitSet(data, CS01) &&
                 bitSet(data, CS02))
            timer->setPrescaler(1024);
        else
            qDebug() << "Unsupported timer mode set!";
    }
    else if (type == IO_OCR0)
        timer->setCompareValue(data);
}

void CTimer0Handler::registerHandler(CBaseIOHandler **array)
{
    array[IO_TCCR0] = this;
    array[IO_OCR0] = this;
}

void CTimer0Handler::initPlugin()
{
    CAVRTimer *timer = getAVRTimer(CAVRClock::TIMER_0);
    timer->setPrescaler(1);
}


void CTimerMaskHandler::handleIOData(EGeneralIOTypes, TGeneralIOData data)
{
    CRP6Simulator::getInstance()->getAVRClock()->enableTimer(CAVRClock::TIMER_0,
                                                             bitSet(data, OCIE0));

    // UNDONE: Other timers
}

void CTimerMaskHandler::registerHandler(CBaseIOHandler **array)
{
    array[IO_TIMSK] = this;
}
