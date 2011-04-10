#include "ioregisters.h"

namespace NRP6SimulGlue {

CIORegister::TSetCallback CIORegister::setCallback = 0;
CIORegister::TGetCallback CIORegister::getCallback = 0;

}

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

// UART
NRP6SimulGlue::CIORegister UCSRA(NRP6SimulGlue::IO_UCSRA);
NRP6SimulGlue::CIORegister UCSRB(NRP6SimulGlue::IO_UCSRB);
NRP6SimulGlue::CIORegister UCSRC(NRP6SimulGlue::IO_UCSRC);
NRP6SimulGlue::CIORegister UDR(NRP6SimulGlue::IO_UDR);
NRP6SimulGlue::CIORegister UBRR(NRP6SimulGlue::IO_UBRR);
NRP6SimulGlue::CIORegister UBRRL(NRP6SimulGlue::IO_UBRRL);
NRP6SimulGlue::CIORegister UBRRH(NRP6SimulGlue::IO_UBRRH);
