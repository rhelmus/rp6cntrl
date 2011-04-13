#ifndef RP6SIMUL_GLUE_H
#define RP6SIMUL_GLUE_H

#include "shared.h"

namespace NRP6SimulGlue {

#define EXPORT extern "C"

void debugOut(const char *s, ...);

}

// ISR
#define ISR(vec) EXPORT void ISR_##vec(void)
void cli(void);
void sei(void);

// progmem
#define PROGMEM /* Nothing */
#define PSTR(s) (s)
#define pgm_read_word(addr) (*addr)
#define pgm_read_byte_near(addr) (*addr)
// ...

// progmem utility functions
#define strcmp_P strcmp
// ...

#include "ioregisters.h"

char *itoa(int value, char *str, int radix);

#endif
