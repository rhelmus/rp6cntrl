#ifndef RP6SIMUL_IOREGISTERS_H
#define RP6SIMUL_IOREGISTERS_H

#include "glue.h"
#include "shared.h"

#include <stdint.h>
#include <time.h>

namespace NRP6SimulGlue {

class CIORegister
{
public:
    typedef TIORegisterData (*TGetCallback)(EIORegisterTypes);
    typedef void (*TSetCallback)(EIORegisterTypes, TIORegisterData);

    static TGetCallback getCallback;
    static TSetCallback setCallback;

private:
    EIORegisterTypes IOType;

    void set(TIORegisterData d) { setCallback(IOType, d); }

public:
    CIORegister(EIORegisterTypes t) : IOType(t) { }
    CIORegister(const CIORegister &other) { set(other.getCallback(IOType)); }

    CIORegister &operator=(TIORegisterData d) { set(d); return *this; }
    CIORegister &operator=(CIORegister &other) { set(other); return *this; }
    operator TIORegisterData(void) { return getCallback(IOType); }
    CIORegister &operator|=(TIORegisterData d)
    { set(getCallback(IOType) | d); return *this; }
    CIORegister &operator&=(TIORegisterData d)
    { set(getCallback(IOType) & d); return *this; }
    CIORegister &operator^=(TIORegisterData d)
    { set(getCallback(IOType) ^ d); return *this; }
};

}

// Timer0
extern NRP6SimulGlue::CIORegister TCCR0;
extern NRP6SimulGlue::CIORegister OCR0;

// Timer1
extern NRP6SimulGlue::CIORegister TCCR1A;
extern NRP6SimulGlue::CIORegister TCCR1B;
extern NRP6SimulGlue::CIORegister OCR1A;
extern NRP6SimulGlue::CIORegister OCR1B;
extern NRP6SimulGlue::CIORegister OCR1AL;
extern NRP6SimulGlue::CIORegister OCR1AH;
extern NRP6SimulGlue::CIORegister OCR1BL;
extern NRP6SimulGlue::CIORegister OCR1BH;
extern NRP6SimulGlue::CIORegister ICR1;

// Timer2
extern NRP6SimulGlue::CIORegister TCCR2;
extern NRP6SimulGlue::CIORegister OCR2;

// TIMSK
extern NRP6SimulGlue::CIORegister TIMSK;

// UART
extern NRP6SimulGlue::CIORegister UCSRA;
extern NRP6SimulGlue::CIORegister UCSRB;
extern NRP6SimulGlue::CIORegister UCSRC;
extern NRP6SimulGlue::CIORegister UDR;
extern NRP6SimulGlue::CIORegister UBRR;
extern NRP6SimulGlue::CIORegister UBRRL;
extern NRP6SimulGlue::CIORegister UBRRH;

// General IO ports
extern NRP6SimulGlue::CIORegister PORTA;
extern NRP6SimulGlue::CIORegister PORTB;
extern NRP6SimulGlue::CIORegister PORTC;
extern NRP6SimulGlue::CIORegister PORTD;
extern NRP6SimulGlue::CIORegister DDRA;
extern NRP6SimulGlue::CIORegister DDRB;
extern NRP6SimulGlue::CIORegister DDRC;
extern NRP6SimulGlue::CIORegister DDRD;
extern NRP6SimulGlue::CIORegister PINA;
extern NRP6SimulGlue::CIORegister PINB;
extern NRP6SimulGlue::CIORegister PINC;
extern NRP6SimulGlue::CIORegister PIND;

// ADC
extern NRP6SimulGlue::CIORegister ADMUX;
extern NRP6SimulGlue::CIORegister ADCSRA;
extern NRP6SimulGlue::CIORegister SFIOR;
extern NRP6SimulGlue::CIORegister ADC;

// External interrupts
extern NRP6SimulGlue::CIORegister MCUCR;
extern NRP6SimulGlue::CIORegister GICR;
extern NRP6SimulGlue::CIORegister MCUCSR;

#endif // RP6SIMUL_IOREGISTERS_H