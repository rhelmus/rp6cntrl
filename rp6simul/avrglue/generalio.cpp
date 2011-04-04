#include "generalio.h"

namespace NRP6SimulGlue {

CGeneralIO::TSetCallback CGeneralIO::setCallback = 0;
CGeneralIO::TGetCallback CGeneralIO::getCallback = 0;
timespec CGeneralIO::lastDelay = { 0, 0 };

void CGeneralIO::checkDelay()
{
    if (!lastDelay.tv_sec && !lastDelay.tv_nsec) // Init?
        clock_gettime(CLOCK_MONOTONIC, &lastDelay);
    else
    {
        timespec current;
        clock_gettime(CLOCK_MONOTONIC, &current);
        long delta = ((current.tv_sec-lastDelay.tv_sec) * 1000000) +
                ((current.tv_nsec-lastDelay.tv_nsec) / 1000);
        if (delta > 10) // Delay every 10 us
        {
            lastDelay = current;
            current.tv_sec = 0; current.tv_nsec = 1000;
            nanosleep(&current, 0);
        }
    }
}

}

// UART
NRP6SimulGlue::CGeneralIO UCSRA(NRP6SimulGlue::IO_UCSRA);
NRP6SimulGlue::CGeneralIO UDR(NRP6SimulGlue::IO_UDR);

// Timer0
NRP6SimulGlue::CGeneralIO TCCR0(NRP6SimulGlue::IO_TCCR0);
NRP6SimulGlue::CGeneralIO OCR0(NRP6SimulGlue::IO_OCR0);

// Timer1
NRP6SimulGlue::CGeneralIO TCCR1A(NRP6SimulGlue::IO_TCCR1A);
NRP6SimulGlue::CGeneralIO TCCR1B(NRP6SimulGlue::IO_TCCR1B);
NRP6SimulGlue::CGeneralIO OCR1A(NRP6SimulGlue::IO_OCR1A);
NRP6SimulGlue::CGeneralIO OCR1B(NRP6SimulGlue::IO_OCR1B);
NRP6SimulGlue::CGeneralIO OCR1AL(NRP6SimulGlue::IO_OCR1AL);
NRP6SimulGlue::CGeneralIO OCR1AH(NRP6SimulGlue::IO_OCR1AH);
NRP6SimulGlue::CGeneralIO OCR1BL(NRP6SimulGlue::IO_OCR1BL);
NRP6SimulGlue::CGeneralIO OCR1BH(NRP6SimulGlue::IO_OCR1BH);
NRP6SimulGlue::CGeneralIO ICR1(NRP6SimulGlue::IO_ICR1);

// Timer2
NRP6SimulGlue::CGeneralIO TCCR2(NRP6SimulGlue::IO_TCCR2);
NRP6SimulGlue::CGeneralIO OCR2(NRP6SimulGlue::IO_OCR2);

// TIMSK
NRP6SimulGlue::CGeneralIO TIMSK(NRP6SimulGlue::IO_TIMSK);
