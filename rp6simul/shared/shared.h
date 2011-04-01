#ifndef RP6SIMUL_SHARED_H
#define RP6SIMUL_SHARED_H

#include <stdint.h>

#ifdef GLUE
namespace NRP6SimulGlue {
#endif

enum EGeneralIOTypes
{
    // 8 bit types
    // ----------------

    // UART
    IO_UCSRA=0,
    IO_UDR,

    // Timer0
    IO_TCCR0,
    IO_OCR0,

    // TIMSK
    IO_TIMSK,

    // 16 bit types
    // ----------------

    // ...

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

// TIMSK
#define OCIE2   7
#define OCIE1A  4
#define OCIE1B  3
#define OCIE0   1


#endif
