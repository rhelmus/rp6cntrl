#include "generalio.h"

namespace NRP6SimulGlue {

CGeneralIO::TSetCallback CGeneralIO::setCallback = 0;
CGeneralIO::TGetCallback CGeneralIO::getCallback = 0;

}

// UART
NRP6SimulGlue::CGeneralIO UCSRA(NRP6SimulGlue::IO_UCSRA);
NRP6SimulGlue::CGeneralIO UDR(NRP6SimulGlue::IO_UDR);

// Timer0
NRP6SimulGlue::CGeneralIO TCCR0(NRP6SimulGlue::IO_TCCR0);
NRP6SimulGlue::CGeneralIO OCR0(NRP6SimulGlue::IO_OCR0);

// TIMSK
NRP6SimulGlue::CGeneralIO TIMSK(NRP6SimulGlue::IO_TIMSK);
