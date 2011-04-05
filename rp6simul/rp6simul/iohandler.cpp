#include "iohandler.h"
#include "avrtimer.h"

#include <QDebug>

#include <QTimer>

#if 0

namespace {

inline bool bitSet(TIORegisterData data, TIORegisterData bit)
{
    return (data & (1<<bit));
}

inline TIORegisterData bitValue(TIORegisterData b)
{
    return (1 << b);
}

inline CAVRTimer *getAVRTimer(CAVRClock::EAVRTimers t)
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
        CRP6Simulator::getInstance()->setIORegister(IO_UDR, *c);
        CRP6Simulator::getInstance()->execISR(ISR_USART_RXC_vect);
    }
}

void CUARTHandler::handleIOData(EIORegisterTypes type, TIORegisterData data)
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
    CRP6Simulator::getInstance()->setIORegister(IO_UCSRA, 1<<5);
}


void CTimer0Handler::handleIOData(EIORegisterTypes type, TIORegisterData data)
{
    CAVRTimer *timer = getAVRTimer(CAVRClock::TIMER_0);
    if (type == IO_TCCR0)
    {
        // UNDONE: Neater error handling
        // UNDONE: More config options handled

        if (bitSet(data, WGM00) || !bitSet(data, WGM01))
            qDebug() << "Unsupported timer0 mode set!";

        if (bitSet(data, COM00) || bitSet(data, COM01))
            qDebug() << "Unsupported COM set for timer0!";

        // Prescaler
        if (!bitSet(data, CS00) && !bitSet(data, CS01) && !bitSet(data, CS02))
            timer->setPrescaler(0);
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
            qDebug() << "Unsupported timer mode set for timer0!";
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


bool CTimer1Handler::PWMEnabled() const
{
    TIORegisterData A = (bitValue(WGM11) & bitValue(COM1A1) & bitValue(COM1B1));
    TIORegisterData B = (bitValue(WGM13) & bitValue(CS10));
    return ((channelA == A) && (channelB == B));
}

int CTimer1Handler::getPrescaler() const
{
    int ret = 1;

    if (!bitSet(channelB, CS10) && !bitSet(channelB, CS11) &&
            !bitSet(channelB, CS12))
        ret = 0;
    else if (bitSet(channelB, CS10) && !bitSet(channelB, CS11) &&
             !bitSet(channelB, CS12))
        ret = 1;
    else if (!bitSet(channelB, CS10) && bitSet(channelB, CS11) &&
             !bitSet(channelB, CS12))
        ret = 8;
    else if (!bitSet(channelB, CS10) && !bitSet(channelB, CS11) &&
             bitSet(channelB, CS12))
        ret = 256;
    else if (bitSet(channelB, CS10) && bitSet(channelB, CS11) &&
             !bitSet(channelB, CS12))
        ret = 64;
    else if (bitSet(channelB, CS10) && !bitSet(channelB, CS11) &&
             bitSet(channelB, CS12))
        ret = 1024;
    else
        qDebug() << "Unsupported timer mode set for timer1!";

    return ret;
}

void CTimer1Handler::updateTimerEnabled()
{
    // UNDONE: Do we actually need this? Don't think it makes sense to set
    // TIMSK when doing PWM?

    CAVRClock *avrclock = CRP6Simulator::getInstance()->getAVRClock();
    bool e = (!PWMEnabled());

    if (e)
        e = bitSet(CRP6Simulator::getInstance()->getIORegister(IO_TIMSK), OCIE1A);

    avrclock->enableTimer(CAVRClock::TIMER_1A, e);
}

void CTimer1Handler::handleIOData(EIORegisterTypes type, TIORegisterData data)
{
    CAVRTimer *timer = getAVRTimer(CAVRClock::TIMER_1A);

    if (type == IO_TCCR1A)
    {
        // UNDONE: Neater error handling
        // UNDONE: More config options handled

        channelA = data;

        if (bitSet(data, WGM10))
            qDebug() << "Unsupported timer1 mode set!";

        if (bitSet(data, COM1A0) || bitSet(data, COM1B0) ||
                (bitSet(data, COM1A1) != bitSet(data, COM1B1)))
            qDebug() << "Unsupported COM set for timer1!";

        updateTimerEnabled();
    }
    else if (type == IO_TCCR1B)
    {
        // UNDONE: Neater error handling
        // UNDONE: More config options handled

        channelB = data;

        // UNDONE: Check for supported bits

        prescaler = getPrescaler();
        timer->setPrescaler(prescaler);
        updateTimerEnabled();
    }
    else if (type == IO_OCR1A)
    {
        timer->setCompareValue(data);
        compareValue = data;
    }
    else if (type == IO_OCR1AL)
    {
        // Assuming data is 16 bits
        compareValue = (data & 0xFF00) + (compareValue & 0x00FF);
        timer->setCompareValue(compareValue);
    }
    else if (type == IO_OCR1AH)
    {
        // Assuming data is 16 bits
        compareValue = (compareValue & 0xFF00) + (data & 0x00FF);
        timer->setCompareValue(compareValue);
    }
    else if (type == IO_ICR1)
    {
        // ...
    }
}

void CTimer1Handler::registerHandler(CBaseIOHandler **array)
{
    array[IO_TCCR1A] = this;
    array[IO_TCCR1B] = this;
    array[IO_OCR1A] = this;
//    array[IO_OCR1B] = this;
    array[IO_OCR1AL] = this;
    array[IO_OCR1AH] = this;
//    array[IO_OCR1BL] = this;
//    array[IO_OCR1BH] = this;
    array[IO_ICR1] = this;
}

void CTimer1Handler::initPlugin()
{
    CAVRTimer *timer = getAVRTimer(CAVRClock::TIMER_1A);
    timer->setPrescaler(1);
    channelA = channelB = 0;
    prescaler = 1;
    compareValue = 0;
}


void CTimer2Handler::handleIOData(EIORegisterTypes type, TIORegisterData data)
{
    CAVRTimer *timer = getAVRTimer(CAVRClock::TIMER_2);
    if (type == IO_TCCR2)
    {
        // UNDONE: Neater error handling
        // UNDONE: More config options handled

        if (bitSet(data, WGM20) || !bitSet(data, WGM21))
            qDebug() << "Unsupported timer2 mode set!";

        if (bitSet(data, COM20) || bitSet(data, COM21))
            qDebug() << "Unsupported COM set for timer2!";

        // Prescaler
        if (!bitSet(data, CS20) && !bitSet(data, CS21) && !bitSet(data, CS22))
            timer->setPrescaler(0);
        else if (bitSet(data, CS20) && !bitSet(data, CS21) &&
                 !bitSet(data, CS22))
            timer->setPrescaler(1);
        else if (!bitSet(data, CS20) && bitSet(data, CS21) &&
                 !bitSet(data, CS22))
            timer->setPrescaler(8);
        else if (!bitSet(data, CS20) && !bitSet(data, CS21) &&
                 bitSet(data, CS22))
            timer->setPrescaler(64);
        else if (bitSet(data, CS20) && bitSet(data, CS21) &&
                 !bitSet(data, CS22))
            timer->setPrescaler(32);
        else if (bitSet(data, CS20) && !bitSet(data, CS21) &&
                 bitSet(data, CS22))
            timer->setPrescaler(128);
        else if (!bitSet(data, CS20) && bitSet(data, CS21) &&
                 bitSet(data, CS22))
            timer->setPrescaler(256);
        else if (bitSet(data, CS20) && bitSet(data, CS21) &&
                 bitSet(data, CS22))
            timer->setPrescaler(1024);
        else
            qDebug() << "Unsupported timer mode set for timer2!";
    }
    else if (type == IO_OCR2)
        timer->setCompareValue(data);
}

void CTimer2Handler::registerHandler(CBaseIOHandler **array)
{
    array[IO_TCCR2] = this;
    array[IO_OCR2] = this;
}

void CTimer2Handler::initPlugin()
{
    CAVRTimer *timer = getAVRTimer(CAVRClock::TIMER_2);
    timer->setPrescaler(1);
}


void CTimerMaskHandler::handleIOData(EIORegisterTypes, TIORegisterData data)
{
    CAVRClock *avrclock = CRP6Simulator::getInstance()->getAVRClock();

    avrclock->enableTimer(CAVRClock::TIMER_0, bitSet(data, OCIE0));
    avrclock->enableTimer(CAVRClock::TIMER_1A, bitSet(data, OCIE1A));
    avrclock->enableTimer(CAVRClock::TIMER_2, bitSet(data, OCIE2));

    // UNDONE: Other timers
}

void CTimerMaskHandler::registerHandler(CBaseIOHandler **array)
{
    array[IO_TIMSK] = this;
}

#endif
