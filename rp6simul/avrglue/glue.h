#ifndef RP6SIMUL_GLUE_H
#define RP6SIMUL_GLUE_H

#include "shared.h"

namespace NRP6SimulGlue {

#define EXPORT extern "C"

extern void *callBackData;

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
// UNDONE: More ...?

// progmem utility functions
#define strcmp_P strcmp
// UNDONE: More ...?

#include "ioregisters.h"

#ifndef _WIN32
char *itoa(int value, char *str, int radix);
#endif

#endif
