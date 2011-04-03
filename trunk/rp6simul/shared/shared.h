#ifndef RP6SIMUL_SHARED_H
#define RP6SIMUL_SHARED_H

#include <stdint.h>

#ifdef GLUE
namespace NRP6SimulGlue {
#endif

enum EGeneralIOTypes
{
    // UART
    IO_UCSRA=0,
    IO_UDR,

    // Timer0
    IO_TCCR0,
    IO_OCR0,

    // Timer 1
    IO_TCCR1A,
    IO_TCCR1B,
    IO_OCR1A,
    IO_OCR1B,
    IO_OCR1AL,
    IO_OCR1AH,
    IO_OCR1BL,
    IO_OCR1BH,
    IO_ICR1,

    // Timer2
    IO_TCCR2,
    IO_OCR2,

    // TIMSK
    IO_TIMSK,

    IO_END
};

// Just use single type that is able to contain both 8 and 16 bit io types
typedef uint16_t TGeneralIOData;

#ifdef GLUE
} // end namespace
#endif

// Macros were copied from avr/iom32.h (avr-libc)

// Timer0
#define FOC0    7
#define WGM00   6
#define COM01   5
#define COM00   4
#define WGM01   3
#define CS02    2
#define CS01    1
#define CS00    0

// Timer1A
#define COM1A1  7
#define COM1A0  6
#define COM1B1  5
#define COM1B0  4
#define WGM11   1
#define WGM10   0

// Timer1B
#define WGM13   4
#define WGM12   3
#define CS12    2
#define CS11    1
#define CS10    0

// Timer2
#define FOC2    7
#define WGM20   6
#define COM21   5
#define COM20   4
#define WGM21   3
#define CS22    2
#define CS21    1
#define CS20    0

// TIMSK
#define OCIE2   7
#define OCIE1A  4
#define OCIE1B  3
#define OCIE0   1


#endif
