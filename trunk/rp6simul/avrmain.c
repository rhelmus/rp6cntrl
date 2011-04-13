#include "glue.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>


#define HEX 16
#define DEC 10
#define OCT 8
#define BIN 2

void writeChar(char ch)
{
    // UNDONE
#define UDRE 5
    while (!(UCSRA & (1<<UDRE)));
    UDR = (uint8_t)ch;
}

void writeString(const char *string)
{
    while(*string)
        writeChar(*string++);
}

void writeInteger(int16_t number, uint8_t base)
{
    char buffer[17];
    itoa(number, &buffer[0], base);
    writeString(&buffer[0]);
}


// UART receive functions:

#define UART_RECEIVE_BUFFER_SIZE 32 // Default buffer size is 32!
#define UART_BUFFER_OK 0
#define UART_BUFFER_OVERFLOW 1


// MAXIMUM Buffer size is 254.
volatile char uart_receive_buffer[UART_RECEIVE_BUFFER_SIZE+1];

volatile uint8_t uart_status;

uint8_t read_pos = 0;
uint8_t write_pos = 0;
uint8_t read_size = 0;
uint8_t write_size = 0;

/**
 * UART receive ISR.
 * Handles reception to circular buffer.
 */
ISR(USART_RXC_vect)
{
    static volatile uint8_t dummy;
    if(((uint8_t)(write_size - read_size)) < UART_RECEIVE_BUFFER_SIZE) {
        uart_receive_buffer[write_pos++] = UDR;
        write_size++;
        if(write_pos > UART_RECEIVE_BUFFER_SIZE)
            write_pos = 0;
        uart_status = UART_BUFFER_OK;
    }
    else {
        dummy = UDR;
        uart_status = UART_BUFFER_OVERFLOW;
    }
}

/**
 * Read a char from the circular buffer.
 * The char is removed from the buffer (or more precise: not accessible directly anymore
 * and will be overwritten as soon as new data becomes available)!
 *
 * Example:
 *
 * // [...]
 * if(getBufferLength())
 *	   receivedData[data_position++] = readChar();
 * // [...]
 *
 */
char readChar(void)
{
    uart_status = UART_BUFFER_OK;
    if(((uint8_t)(write_size - read_size)) > 0) {
        read_size++;
        if(read_pos > UART_RECEIVE_BUFFER_SIZE)
            read_pos = 0;
        return uart_receive_buffer[read_pos++];
    }
    return 0;
}

/**
 * Same as readChar, but this function copies numberOfChars chars from the
 * circular buffer to buf.
 * It also returns the number of characters really copied to the buffer!
 * Just in case that there were fewer chars in the buffer...
 */
uint8_t readChars(char *buf, uint8_t numberOfChars)
{
    uint8_t i = 0;
    uart_status = UART_BUFFER_OK;
    while(((uint8_t)(write_size - read_size)) >= numberOfChars) {
        read_size++;
        buf[i++] = uart_receive_buffer[read_pos++];
        if(read_pos > UART_RECEIVE_BUFFER_SIZE)
            read_pos = 0;
    }
    return i;
}

/**
 * Returns the current number of elements in the buffer.
 *
 * Example:
 * s. readChar function above!
 */
uint8_t getBufferLength(void)
{
    return (((uint8_t)(write_size - read_size)));
}

/**
 * Clears the reception buffer - it disables UART Receive
 * interrupt for a short period of time.
 */
void clearReceptionBuffer(void)
{
    static uint8_t dummy;
//    UCSRB &= ~(1 << RXCIE); // disable UART RX Interrupt
    dummy = UDR;
    read_pos = 0;
    write_pos = 0;
    read_size = 0;
    write_size = 0;
    uart_status = UART_BUFFER_OK;
//    UCSRB |= (1 << RXCIE); // enable Interrupt again
}

#include <time.h>

volatile uint16_t delay_timer;

ISR (TIMER0_COMP_vect)
{
    delay_timer++;

    static timespec start, end;
    static bool init = true;
    static uint32_t total = 0, count = 0;

    if (init)
    {
        clock_gettime(CLOCK_MONOTONIC, &start);
        init = false;
    }
    else
    {
        clock_gettime(CLOCK_MONOTONIC, &end);
        uint32_t delta = ((end.tv_sec-start.tv_sec) * 1000000) +
                ((end.tv_nsec-start.tv_nsec) / 1000);
        start = end;

        total += delta;
        count++;

        if (total > 1000000)
        {
            printf("avg-delta: %d\n", total / count);
            fflush(stdout);
            total = count = 0;
        }
    }

//    UDR = UDR + 1;
}

void uSleep(uint8_t time)
{
    for (delay_timer = 0; delay_timer < time;);
}

void mSleep(uint16_t time)
{
    while (time--) uSleep(10);
}

volatile uint32_t delay_timer_high;


ISR (TIMER2_COMP_vect)
{
    static timespec start, end;
    static bool init = true;

    delay_timer_high++;

    if (init)
    {
        clock_gettime(CLOCK_MONOTONIC, &start);
        init = false;
    }
    else if (delay_timer_high >= 72000)
    {
        clock_gettime(CLOCK_MONOTONIC, &end);
        uint32_t delta = ((end.tv_sec-start.tv_sec) * 1000000) +
                ((end.tv_nsec-start.tv_nsec) / 1000);
        start = end;

        printf("timer2 res: %d\n", delta);
        fflush(stdout);
        delay_timer_high = 0;
    }
}


// ---------------------------------------------------
// PORTA A/D Convertor channels

#define ADC_BAT 			7
#define ADC_MCURRENT_L 		6
#define ADC_MCURRENT_R 		5
#define ADC_LS_L 			3
#define ADC_LS_R 			2
#define ADC_ADC1 			1
#define ADC_ADC0 			0

/**
 * This function starts an ADC conversion - it does not return the
 * read value! You need to poll if the conversion is complete somewhere
 * else and then read it from the ADC result register.
 * (s. task_ADC function below)
 */
void startADC(uint8_t channel)
{
    ADMUX = (1<<REFS0) | (0<<REFS1) | (channel<<MUX0);
    ADCSRA = (0<<ADIE) | (1<<ADSC) | (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADIF);
}

// -----------------------

uint16_t adcBat;
uint16_t adcMotorCurrentLeft;
uint16_t adcMotorCurrentRight;
uint16_t adcLSL;
uint16_t adcLSR;
uint16_t adc0;
uint16_t adc1;

/**
 * This functions checks all ADC channels sequentially in the Background!
 * It can save a lot of time, if the ADC channels are checked like this, because
 * each A/D conversion takes some time. With this function you don't need to
 * wait until the A/D conversion is finished and you can do other things in the
 * meanwhile.
 * If you use this function (this is also the case if you use task_RP6System
 * because it calls this function), you can NOT use readADC anymore!
 *
 * Instead you can use the seven global variables you see above to
 * get the ADC values!
 */
void task_ADC(void)
{
    static uint8_t current_adc_channel = 0;
    if(!(ADCSRA & (1<<ADSC))) {
    //	ADCSRA |= (1<<ADIF);
        switch(current_adc_channel) {
            case 0: adcBat = ADC; startADC(ADC_MCURRENT_L); break;
            case 1: adcMotorCurrentLeft = ADC; startADC(ADC_MCURRENT_R); break;
            case 2: adcMotorCurrentRight = ADC; startADC(ADC_LS_L); break;
            case 3: adcLSL = ADC; startADC(ADC_LS_R); break;
            case 4: adcLSR = ADC; startADC(ADC_ADC0); break;
            case 5: adc0 = ADC; startADC(ADC_ADC1); break;
            case 6: adc1 = ADC; startADC(ADC_BAT); break;
        }
        if(current_adc_channel == 6)
            current_adc_channel = 0;
        else
            current_adc_channel++;
    }
}

int main()
{
    // UART:
#define F_CPU 8000000
#define BAUD_LOW		38400  //Low speed - 38.4 kBaud
#define UBRR_BAUD_LOW	((F_CPU/(16*BAUD_LOW))-1)

    UBRRH = UBRR_BAUD_LOW >> 8;	// Setup UART: Baudrate is Low Speed
    UBRRL = (uint8_t) UBRR_BAUD_LOW;
    UCSRA = 0x00;
    UCSRC = (1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0);
    UCSRB = (1 << TXEN) | (1 << RXEN) | (1 << RXCIE);

    TCCR0 =   (0 << WGM00) | (1 << WGM01)
            | (0 << COM00) | (0 << COM01)
            | (0 << CS02)  | (1 << CS01) | (0 << CS00);
//    OCR0  = 99;
    OCR0 = 8;

    TCCR2 = (1 << WGM21) | (0 << COM20) | (1 << CS20);
    OCR2  = 0x6E; // 0x6E = 72kHz @8MHz

    TCCR1A = (0 << WGM10) | (1 << WGM11) | (1 << COM1A1) | (1 << COM1B1);
    TCCR1B =  (1 << WGM13) | (0 << WGM12) | (1 << CS10);
    ICR1 = 210; // Phase corret PWM top value - 210 results in
                // about 19 kHz PWM.
                // ICR1 is the maximum (=100% duty cycle) PWM value!
                // This means that the PWM resolution is a bit lower, but
                // if the frequency is lower than 19 kHz you may hear very
                // annoying high pitch noises from the motors!
                // 19 kHz is a bit over the maximum frequency most people can
                // hear!
                //
                // ATTENTION: Max PWM value is 210 and NOT 255 !!!
    OCR1AL = 0;
    OCR1BL = 0;

    // Timer 1: Normal port operation, mode 4 (CTC), clk/8
    /*TCCR1A =  (0 << COM1A1)
          | (0 << COM1A0)
          | (0 << COM1B1)
          | (0 << COM1B0)
          | (0 << FOC1A)
          | (0 << FOC1B)
          | (0 << WGM11)
          | (0 << WGM10);
    TCCR1B =  (0 << ICNC1)
          | (0 << ICES1)
          | (0 << WGM13)
          | (1 << WGM12)
          | (0 << CS12)
          | (1 << CS11)
          | (0 << CS10);
    OCR1A = 9;*/

    //TIMSK = (1 << OCIE0) | (1 << OCIE1A) | (1 << OCIE2);
    TIMSK = (1 << OCIE0) | (1 << OCIE2);

    PORTA =     0b11010010;
    DDRD =      0b11001100;
    PINC =      0b10010111;

    // Enable LED 4
    DDRB |= (1<<PINB7); PORTB |= (1 << PINB7);

    for (;;)
    {
        printf("ADC:\n\tBat: %d\n\tCurrent left: %d\n\tCurrent right: %d\n\t"
               "LSL: %d\n\tLSR: %d\n\tadc0: %d\n\tadc1: %d\n", adcBat,
               adcMotorCurrentLeft, adcMotorCurrentRight, adcLSL, adcLSR, adc0, adc1);
        fflush(stdout);
        task_ADC();
        mSleep(500);
    }
    return 0;
}

