#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <stdarg.h>
#include <stdlib.h>

#include "animate.h"

// Bit manipulation macros
#define sbi(a, b) ((a) |= 1 << (b))       //sets bit B in variable A
#define cbi(a, b) ((a) &= ~(1 << (b)))    //clears bit B in variable A
#define tbi(a, b) ((a) ^= 1 << (b))       //toggles bit B in variable A

#define BAUD 38400
#define UBBR (F_CPU / 16 / BAUD - 1)
#define TIMER0_INIT 235 // 20 ticks before it overflows
#define DEBUG 0

#define NUM_LED 4
#define NUM_DATA (NUM_LED * 3)

#define COLOR_LATCH_DURATION 501
#define CLOCK_PERIOD 100
#define CLOCK_PIN 2
#define DATA_PIN 3

// Time keeping
static volatile uint32_t g_time = 0;

// are we currently transmitting?
static volatile uint8_t  g_transmit = 0;


// the data we're shifting out
static volatile uint8_t  g_data_index = 0;
static volatile uint8_t  g_clock_state = 1;
static volatile uint8_t  g_data_byte_offset = 0;
static volatile uint8_t  g_data[NUM_DATA];
static volatile uint8_t  g_current_byte;

// timer interrupt. 
ISR (TIMER0_OVF_vect)
{
    g_time++;

    if (!g_transmit)
        return;

    if (g_clock_state)
    {
        // set clock low
        cbi(PORTD, CLOCK_PIN);

        if (g_data_byte_offset == 0)
        {
            // Are we done?
            if (g_data_index == NUM_DATA)
            {
                g_transmit = 0;
//                g_data_index = 0;
//                g_data_byte_offset = 0;
                g_clock_state = 1;
                // Set the clock to low
                cbi(PORTD, CLOCK_PIN);
                cbi(PORTB, 3);

                //TODO: Set next row time
                return;
            }
            g_current_byte = g_data[g_data_index++];
        }
        if (g_current_byte & (1 << (8 - g_data_byte_offset)))
            sbi(PORTD, DATA_PIN);
        else
            cbi(PORTD, DATA_PIN);
        g_data_byte_offset = (g_data_byte_offset + 1) % 8;
        g_clock_state = 0;
    }
    else
    {
        g_clock_state = 1;
        sbi(PORTD, CLOCK_PIN);
    }
}

void serial_init(void)
{
    /*Set baud rate */ 
    UBRR0H = (unsigned char)(UBBR>>8); 
    UBRR0L = (unsigned char)UBBR; 
    /* Enable transmitter */ 
    UCSR0B = (1<<TXEN0) | (1<<RXEN0) | (1<<RXCIE0);
    /* Set frame format: 8data, 1stop bit */ 
    UCSR0C = (0<<USBS0)|(3<<UCSZ00); 
}

void serial_tx(uint8_t ch)
{
    while ( !( UCSR0A & (1<<UDRE0)) )
        ;
    UDR0 = ch;
}

#define MAX 80 

// debugging printf function. Max MAX characters per line!!
void dprintf(const char *fmt, ...)
{
    va_list va;
    va_start (va, fmt);
    char buffer[MAX];
    char *ptr = buffer;
    vsnprintf(buffer, MAX, fmt, va);
    va_end (va);
    for(ptr = buffer; *ptr; ptr++)
    {
        if (*ptr == '\n') serial_tx('\r');
        serial_tx(*ptr);
    }
}

uint8_t serial_rx(void)
{
    while ( !(UCSR0A & (1<<RXC0))) 
          ;
        
    return UDR0;
}

void ledstick_setup(void)
{
    // PD2 - pin 2 - clock
    // PD3 - pin 3 - signal
    DDRD |= (1<<PD2)|(1<<PD3);

    // on board LED
    DDRB |= (1<<PB5);

    // Timer setup for clock 
    TCCR0B |= _BV(CS00); // clock / 256 = 16us
    //TCCR0B |= _BV(CS02) | _BV(CS00); // clock / 1024 / 256 = 16us
    //TIMSK0 |= (1<<TOIE0);

    serial_init();
}

void set_led_colors(unsigned char leds[NUM_LED * 3])
{
    uint8_t i, l, c, byte;

    for(l = 0; l < NUM_LED; l++)
        for(c = 0; c < 3; c++)
            {
                byte = leds[(l * 3) + c];
                for(i = 0; i < 8; i++)
                {
                    if (byte & (1 << (8 - i)))
                        sbi(PORTD, DATA_PIN);
                    else
                        cbi(PORTD, DATA_PIN);
                    _delay_us(CLOCK_PERIOD);

                    sbi(PORTD, CLOCK_PIN);
                    _delay_us(CLOCK_PERIOD);

                    cbi(PORTD, CLOCK_PIN);
                }
            }
     _delay_us(COLOR_LATCH_DURATION);
}

int main(void)
{
    ledstick_setup();
    startup();
    fade_in();
    rainbow();
}
