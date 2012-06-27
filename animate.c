#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <avr/delay.h>
#include "hue.h"
#include "ledstick.h"

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
    int     i, j;
    color_t led;
    unsigned char leds[NUM_LED * 3];
 
    for(;;)
        for(i = 0; i < HUE_MAX; i++)
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
    int     i, j;
    unsigned char leds[NUM_LED * 3];
 
    for(i = 0; i < 255; i++)
    {
        for(j = 0; j < NUM_LED; j++)
        {
            leds[(j * 3)] = i;
            leds[(j * 3) + 1] = i;
            leds[(j * 3) + 2] = i;
        }
        set_led_colors(leds);
        _delay_us(10000);
    }
}

int _main(int argc, char *argv[])
{
    ledstick_setup();
    rainbow();
    return 0;
}
