#ifndef RP6SIMUL_SHARED_H
#define RP6SIMUL_SHARED_H

#include <stdint.h>

// NOTE: When adding stuff here, don't forget to set them also for Lua!

#ifdef GLUE
namespace NRP6SimulGlue {
#endif

enum EIORegisterTypes
{
    // UART
    IO_UCSRA=0,
    IO_UCSRB,
    IO_UCSRC,
    IO_UBRR,
    IO_UBRRL,
    IO_UBRRH,
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
typedef uint16_t TIORegisterData;

#ifdef GLUE
} // end namespace
#endif

// Macros were copied from avr/iom32.h (avr-libc)

// Timer0
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
#define FOC1A   3
#define FOC1B   2
#define WGM11   1
#define WGM10   0

// Timer1B
#define ICNC1   7
#define ICES1   6
#define WGM13   4
#define WGM12   3
#define CS12    2
#define CS11    1
#define CS10    0

// Timer2
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

// UCSRA
#define RXC     7
#define TXC     6
#define UDRE    5
#define FE      4
#define DOR     3
#define PE      2
#define U2X     1
#define MPCM    0

// UCSRB
#define RXCIE   7
#define TXCIE   6
#define UDRIE   5
#define RXEN    4
#define TXEN    3
#define UCSZ2   2
#define RXB8    1
#define TXB8    0

// UCSRC
#define URSEL   7
#define UMSEL   6
#define UPM1    5
#define UPM0    4
#define USBS    3
#define UCSZ1   2
#define UCSZ0   1
#define UCPOL   0


#endif
