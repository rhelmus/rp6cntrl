#ifndef RP6SIMUL_GENERALIO_H
#define RP6SIMUL_GENERALIO_H

#include "glue.h"
#include "shared.h"

#include <stdint.h>
#include <time.h>

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

    static timespec lastDelay;

    CGeneralIO(const CGeneralIO &); // No copying

    void set(TGeneralIOData d) { setCallback(IOType, d); checkDelay(); }
    void checkDelay(void); // Relieves CPU a bit

public:
    CGeneralIO(EGeneralIOTypes t) : IOType(t) { }

    CGeneralIO &operator=(TGeneralIOData d) { set(d); return *this; }
    CGeneralIO &operator=(CGeneralIO &other) { set(other); return *this; }
    operator TGeneralIOData(void) { checkDelay(); return getCallback(IOType); }
};

}

// UART
extern NRP6SimulGlue::CGeneralIO UCSRA;
extern NRP6SimulGlue::CGeneralIO UDR;

// Timer0
extern NRP6SimulGlue::CGeneralIO TCCR0;
extern NRP6SimulGlue::CGeneralIO OCR0;

// Timer1
extern NRP6SimulGlue::CGeneralIO TCCR1A;
extern NRP6SimulGlue::CGeneralIO TCCR1B;
extern NRP6SimulGlue::CGeneralIO OCR1A;
extern NRP6SimulGlue::CGeneralIO OCR1B;
extern NRP6SimulGlue::CGeneralIO OCR1AL;
extern NRP6SimulGlue::CGeneralIO OCR1AH;
extern NRP6SimulGlue::CGeneralIO OCR1BL;
extern NRP6SimulGlue::CGeneralIO OCR1BH;
extern NRP6SimulGlue::CGeneralIO ICR1;

// Timer2
extern NRP6SimulGlue::CGeneralIO TCCR2;
extern NRP6SimulGlue::CGeneralIO OCR2;

// TIMSK
extern NRP6SimulGlue::CGeneralIO TIMSK;

#endif // RP6SIMUL_GENERALIO_H
