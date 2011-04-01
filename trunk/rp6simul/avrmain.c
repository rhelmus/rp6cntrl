#include "glue.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

// From: http://www.koders.com/c/fidC5846709A73E0878A830C5F27B617108633E27C5.aspx

#define NUMBER_OF_DIGITS 16   /* space for NUMBER_OF_DIGITS + '\0' */

void uitoa(unsigned int value, char* string, int radix)
{
unsigned char index, i;

  index = NUMBER_OF_DIGITS;
  i = 0;

  do {
    string[--index] = '0' + (value % radix);
    if ( string[index] > '9') string[index] += 'A' - ':';   /* continue with A, B,.. */
    value /= radix;
  } while (value != 0);

  do {
    string[i++] = string[index++];
  } while ( index < NUMBER_OF_DIGITS );

  string[i] = 0; /* string terminator */
}

void itoa(int value, char* string, int radix)
{
  if (value < 0 && radix == 10) {
    *string++ = '-';
    uitoa(-value, string, radix);
  }
  else {
    uitoa(value, string, radix);
  }
}
// ------------------------


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
}

void uSleep(uint8_t time)
{
    for (delay_timer = 0; delay_timer < time;);
}

void mSleep(uint16_t time)
{
    while (time--) uSleep(10);
}

int main()
{
    // Init
//    UCSRA = 0;

    TCCR0 =   (0 << WGM00) | (1 << WGM01)
            | (0 << COM00) | (0 << COM01)
            | (0 << CS02)  | (1 << CS01) | (0 << CS00);
    OCR0  = 99;
    TIMSK = (1 << OCIE0);

    for (;;)
    {
        timespec start, end;

        clock_gettime(CLOCK_REALTIME, &start);
        mSleep(1000);
        clock_gettime(CLOCK_REALTIME, &end);

        printf("delta: %d\n", ((end.tv_sec-start.tv_sec) * 1000000) +
               ((end.tv_nsec-start.tv_nsec) / 1000));
        fflush(stdout);

//        writeString("UART test :-)\n");
//        sleep(1);
//        while (!getBufferLength()) ;
//        writeString("Received char: ");
//        writeChar(readChar());
//        writeChar('\n');


    }
    return 0;
}

