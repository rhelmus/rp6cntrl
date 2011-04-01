#ifndef RP6SIMUL_GLUE_H
#define RP6SIMUL_GLUE_H

#include "shared.h"

namespace NRP6SimulGlue {

#define EXPORT extern "C"

void debugOut(const char *s, ...);

}

#define ISR(vec) EXPORT void ISR_##vec(void)

#include "generalio.h"

#endif
