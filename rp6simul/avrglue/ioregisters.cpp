#include "ioregisters.h"

namespace NRP6SimulGlue {

CIORegister::TSetCallback CIORegister::setCallback = 0;
CIORegister::TGetCallback CIORegister::getCallback = 0;
timespec CIORegister::lastDelay = { 0, 0 };

void CIORegister::checkDelay()
{return;
    if (!lastDelay.tv_sec && !lastDelay.tv_nsec) // Init?
        clock_gettime(CLOCK_MONOTONIC, &lastDelay);
    else
    {
        timespec current;
        clock_gettime(CLOCK_MONOTONIC, &current);
        long delta = ((current.tv_sec-lastDelay.tv_sec) * 1000000) +
                ((current.tv_nsec-lastDelay.tv_nsec) / 1000);
        if (delta > 50) // Delay every 50 us
        {
            lastDelay = current;
            current.tv_sec = 0; current.tv_nsec = 1000;
            nanosleep(&current, 0);
        }
    }
}

}

// UART
NRP6SimulGlue::CIORegister UCSRA(NRP6SimulGlue::IO_UCSRA);
NRP6SimulGlue::CIORegister UDR(NRP6SimulGlue::IO_UDR);

// Timer0
NRP6SimulGlue::CIORegister TCCR0(NRP6SimulGlue::IO_TCCR0);
NRP6SimulGlue::CIORegister OCR0(NRP6SimulGlue::IO_OCR0);

// Timer1
NRP6SimulGlue::CIORegister TCCR1A(NRP6SimulGlue::IO_TCCR1A);
NRP6SimulGlue::CIORegister TCCR1B(NRP6SimulGlue::IO_TCCR1B);
NRP6SimulGlue::CIORegister OCR1A(NRP6SimulGlue::IO_OCR1A);
NRP6SimulGlue::CIORegister OCR1B(NRP6SimulGlue::IO_OCR1B);
NRP6SimulGlue::CIORegister OCR1AL(NRP6SimulGlue::IO_OCR1AL);
NRP6SimulGlue::CIORegister OCR1AH(NRP6SimulGlue::IO_OCR1AH);
NRP6SimulGlue::CIORegister OCR1BL(NRP6SimulGlue::IO_OCR1BL);
NRP6SimulGlue::CIORegister OCR1BH(NRP6SimulGlue::IO_OCR1BH);
NRP6SimulGlue::CIORegister ICR1(NRP6SimulGlue::IO_ICR1);

// Timer2
NRP6SimulGlue::CIORegister TCCR2(NRP6SimulGlue::IO_TCCR2);
NRP6SimulGlue::CIORegister OCR2(NRP6SimulGlue::IO_OCR2);

// TIMSK
NRP6SimulGlue::CIORegister TIMSK(NRP6SimulGlue::IO_TIMSK);
