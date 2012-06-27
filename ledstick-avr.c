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

// what light pattern should we show now?
static volatile uint8_t g_received = 0;

// some prototypes
uint8_t should_break(void);

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

ISR(USART_RX_vect) 
{ 
    g_received = UDR0;
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

void startup(void)
{
    int i;

    unsigned char leds[NUM_LED * 3] = { 0xff, 0xff, 0x00, 
                                        0xff, 0x00, 0xff,
                                        0xff, 0xff, 0x00,
                                        0xff, 0x00, 0xff };
    unsigned char leds2[NUM_LED * 3] = { 0xff, 0x00, 0xff, 
                                         0xff, 0xff, 0x00,
                                         0xff, 0x00, 0xff,
                                         0xff, 0xff, 0x00 };

    for(i = 0; i < 3; i++)
    {
        set_led_colors(leds);
        _delay_ms(100);

        set_led_colors(leds2);
        _delay_ms(100);
    }
}

void rainbow(void)
{
    uint8_t i, j;
    color_t led;
    uint8_t leds[NUM_LED * 3];
 
    for(; !should_break();)
        for(i = 0; i < HUE_MAX && !should_break(); i++)
        {
            for(j = 0; j < NUM_LED; j++)
            {
                color_hue((i + j) % HUE_MAX, &led);
                leds[(j * 3)] = led.red;
                leds[(j * 3) + 1] = led.green;
                leds[(j * 3) + 2] = led.blue;
            }
            set_led_colors(leds);
            _delay_ms(10);
        }
}

void fade_in(void)
{
    uint8_t i, j;
    uint8_t leds[NUM_LED * 3];
 
    for(i = 0; !should_break(); i++)
    {
        for(j = 0; j < NUM_LED; j++)
        {
            leds[(j * 3)] = i;
            leds[(j * 3) + 1] = 0;
            leds[(j * 3) + 2] = 0;
        }
        set_led_colors(leds);
        _delay_ms(2);
    }
}

uint8_t should_break(void)
{
    uint8_t r;

    cli();
    r = g_received;
    sei();

    return r;
}

int main(void)
{
    uint8_t ch;

    ledstick_setup();
    dprintf("ledstick starting\n");
    sei();
    startup();
    for(;;)
    {
        ch = should_break();
        if (ch)
        {
            cli();
            g_received = 0;
            sei();
            dprintf("received '%d'\n", ch);
        }
        switch(ch)
        {
            case 'f':
                dprintf("fade in\n");
                fade_in();
                break;

            case 'r':
            default:
                dprintf("rainbow\n");
                rainbow();
                dprintf("post rainbow\n");
                break;
        }
    }
}
