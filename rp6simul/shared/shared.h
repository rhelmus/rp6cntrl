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

    // General IO
    IO_PORTA,
    IO_PORTB,
    IO_PORTC,
    IO_PORTD,
    IO_DDRA,
    IO_DDRB,
    IO_DDRC,
    IO_DDRD,
    IO_PINA,
    IO_PINB,
    IO_PINC,
    IO_PIND,

    // ADC
    IO_ADMUX,
    IO_ADCSRA,
    IO_SFIOR,
    IO_ADC,

    // External interrupts
    IO_MCUCR,
    IO_GICR,
    IO_MCUCSR,

    // SPI
    IO_SPCR,
    IO_SPSR,
    IO_SPDR,

    IO_END
};

// Just use single type that is able to contain both 8 and 16 bit io types
typedef uint16_t TIORegisterData;

typedef void (*TIORegisterSetCB)(EIORegisterTypes, TIORegisterData, void *);
typedef TIORegisterData (*TIORegisterGetCB)(EIORegisterTypes, void *);
typedef void (*TEnableISRsCB)(bool, void *);
typedef void (*TSetPluginCallbacks)(TIORegisterSetCB, TIORegisterGetCB,
                                    TEnableISRsCB, void *);
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

// PORTA
#define PA7     7
#define PA6     6
#define PA5     5
#define PA4     4
#define PA3     3
#define PA2     2
#define PA1     1
#define PA0     0

// DDRA
#define DDA7    7
#define DDA6    6
#define DDA5    5
#define DDA4    4
#define DDA3    3
#define DDA2    2
#define DDA1    1
#define DDA0    0

// PINA
#define PINA7   7
#define PINA6   6
#define PINA5   5
#define PINA4   4
#define PINA3   3
#define PINA2   2
#define PINA1   1
#define PINA0   0

// PORTB
#define PB7     7
#define PB6     6
#define PB5     5
#define PB4     4
#define PB3     3
#define PB2     2
#define PB1     1
#define PB0     0

// DDRB
#define DDB7    7
#define DDB6    6
#define DDB5    5
#define DDB4    4
#define DDB3    3
#define DDB2    2
#define DDB1    1
#define DDB0    0

// PINB
#define PINB7   7
#define PINB6   6
#define PINB5   5
#define PINB4   4
#define PINB3   3
#define PINB2   2
#define PINB1   1
#define PINB0   0

// PORTC
#define PC7     7
#define PC6     6
#define PC5     5
#define PC4     4
#define PC3     3
#define PC2     2
#define PC1     1
#define PC0     0

// DDRC
#define DDC7    7
#define DDC6    6
#define DDC5    5
#define DDC4    4
#define DDC3    3
#define DDC2    2
#define DDC1    1
#define DDC0    0

// PINC
#define PINC7   7
#define PINC6   6
#define PINC5   5
#define PINC4   4
#define PINC3   3
#define PINC2   2
#define PINC1   1
#define PINC0   0

// PORTD
#define PD7     7
#define PD6     6
#define PD5     5
#define PD4     4
#define PD3     3
#define PD2     2
#define PD1     1
#define PD0     0

// DDRD
#define DDD7    7
#define DDD6    6
#define DDD5    5
#define DDD4    4
#define DDD3    3
#define DDD2    2
#define DDD1    1
#define DDD0    0

// PIND
#define PIND7   7
#define PIND6   6
#define PIND5   5
#define PIND4   4
#define PIND3   3
#define PIND2   2
#define PIND1   1
#define PIND0   0

// ADCSRA
#define ADEN    7
#define ADSC    6
#define ADATE   5
#define ADIF    4
#define ADIE    3
#define ADPS2   2
#define ADPS1   1
#define ADPS0   0

// ADMUX
#define REFS1   7
#define REFS0   6
#define ADLAR   5
#define MUX4    4
#define MUX3    3
#define MUX2    2
#define MUX1    1
#define MUX0    0

// GICR
#define INT1    7
#define INT0    6
#define INT2    5
#define IVSEL   1
#define IVCE    0

// MCUCR
#define SE      7
#define SM2     6
#define SM1     5
#define SM0     4
#define ISC11   3
#define ISC10   2
#define ISC01   1
#define ISC00   0

// MCUCSR
#define JTD     7
#define ISC2    6
#define JTRF    4
#define WDRF    3
#define BORF    2
#define EXTRF   1
#define PORF    0

// SPSR
#define SPIF    7
#define WCOL    6
#define SPI2X   0

// SPCR
#define SPIE    7
#define SPE     6
#define DORD    5
#define MSTR    4
#define CPOL    3
#define CPHA    2
#define SPR1    1
#define SPR0    0


#endif
