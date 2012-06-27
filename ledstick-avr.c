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

#include "hue.h"

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

void setup(void)
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

void on()
{
    uint8_t  i;

    cbi(PORTD, CLOCK_PIN);
    sbi(PORTD, DATA_PIN);
    _delay_us(5);
    for(i = 0; i < 200; i++)
    {
        sbi(PORTD, CLOCK_PIN);
        _delay_us(5);

        cbi(PORTD, CLOCK_PIN);
        tbi(PORTD, DATA_PIN);
        _delay_us(5);
    }
    _delay_us(500);
    dprintf("done!\n");
}

void off()
{
    uint8_t  i;

    cbi(PORTD, CLOCK_PIN);
    cbi(PORTD, DATA_PIN);
    _delay_us(5);
    for(i = 0; i < 200; i++)
    {
        sbi(PORTD, CLOCK_PIN);
        _delay_us(5);

        cbi(PORTD, CLOCK_PIN);
        _delay_us(5);
    }
    _delay_us(500);
    dprintf("done!\n");
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

void set_led_bork(uint8_t *data)
{
    uint8_t  i, index = 0, offset = 0;

    cbi(PORTD, CLOCK_PIN);
    cbi(PORTD, DATA_PIN);
    _delay_us(5);
    for(i = 0; i < NUM_DATA; i++)
    {
        if (data[index] & offset)
            sbi(PORTD, DATA_PIN);
        else
            cbi(PORTD, DATA_PIN);

        _delay_us(5);
        sbi(PORTD, CLOCK_PIN);
        _delay_us(5);

        cbi(PORTD, CLOCK_PIN);

        offset++;
        if (offset == 8)
        {
            index++;
            offset = 0;
            if (index == NUM_DATA)
                break;
        }
    }
    cbi(PORTD, DATA_PIN);

    for(i = 0; i < 10; i++)
    {
        _delay_us(5);
        tbi(PORTD, CLOCK_PIN);
    }
    _delay_us(5);
    cbi(PORTD, CLOCK_PIN);

    _delay_ms(1);
}

void set_leds_int(uint8_t *data)
{
    uint8_t tr;

    memcpy((void *)g_data, data, NUM_DATA);
    g_transmit = 1;
    for(;;)
    {
        cli();
        tr = g_transmit;
        sei();
        if (!tr) 
            break;
    }
    _delay_us(500);
}

int main(void)
{
    uint8_t  i, j;
    color_t  c[NUM_LED];
    uint8_t  d[NUM_DATA];

    setup();
    sei();

    for(i = 0; i < 3; i++)
    {
        on();
        _delay_ms(100);
        off();
        _delay_ms(100);
    }

    dprintf("ledstick hello!\n");
    memset(d, 0, sizeof(d));
    for(i = 128; i < 255; i++)
    {
        for(j = 0; j < NUM_LED; j++)
        {
            //color_hue(i, &c[j]);
            //c[j].red = i;
            //c[j].green = 0;
            //c[j].blue = 0;
            d[j * 3] = i;
            d[j * 3 + 1] = i;
            d[j * 3 + 2] = i;
        }
        set_led_colors((uint8_t *)&d);
        _delay_ms(50);
    }
}
