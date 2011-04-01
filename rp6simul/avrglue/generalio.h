#ifndef RP6SIMUL_GENERALIO_H
#define RP6SIMUL_GENERALIO_H

#include "glue.h"
#include "shared.h"

#include <stdint.h>

namespace NRP6SimulGlue {

class CGeneralIO
{
public:
    typedef TGeneralIOData (*TGetCallback)(EGeneralIOTypes);
    typedef void (*TSetCallback)(EGeneralIOTypes, TGeneralIOData);

    static TGetCallback getCallback;
    static TSetCallback setCallback;

private:
    EGeneralIOTypes IOType;

    CGeneralIO(const CGeneralIO &); // No copying

    void set(TGeneralIOData d) { setCallback(IOType, d); }

public:
    CGeneralIO(EGeneralIOTypes t) : IOType(t) { }

    CGeneralIO &operator=(TGeneralIOData d) { set(d); return *this; }
    CGeneralIO &operator=(CGeneralIO &other) { set(other); return *this; }
    operator TGeneralIOData(void) { return getCallback(IOType); }
};

}

// UART
extern NRP6SimulGlue::CGeneralIO UCSRA;
extern NRP6SimulGlue::CGeneralIO UDR;

// Timer0
extern NRP6SimulGlue::CGeneralIO TCCR0;
extern NRP6SimulGlue::CGeneralIO OCR0;

// TIMSK
extern NRP6SimulGlue::CGeneralIO TIMSK;

#endif // RP6SIMUL_GENERALIO_H
