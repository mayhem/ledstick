#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <bcm2835.h>
#include "hue.c"

// pin definitions
// Power: 5v at P1-02
// Ground:      P1-06
// Data:        P1-
// Clock:       P1-

#define DATA_PIN  RPI_GPIO_P1_21
#define CLOCK_PIN RPI_GPIO_P1_23

#define NUM_LED              4
#define COLOR_LATCH_DURATION 501

void setup(void)
{
    if (!bcm2835_init())
	return;

    bcm2835_gpio_fsel(DATA_PIN, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(CLOCK_PIN, BCM2835_GPIO_FSEL_OUTP);

    // Reset the LED chain
    bcm2835_gpio_write(CLOCK_PIN, 0);
    usleep(COLOR_LATCH_DURATION);
}

void set_led_colors(unsigned char leds[NUM_LED * 3])
{
    int i, l, c;
    unsigned char byte;
    for(l = 0; l < NUM_LED; l++)
        for(c = 0; c < 3; c++)
            {
                byte = leds[(l * 3) + c];
                for(i = 0; i < 8; i++)
                {
                    bcm2835_gpio_write(DATA_PIN, byte & (1 << (8 - i)));
                    usleep(50);

                    bcm2835_gpio_write(CLOCK_PIN, 1);
                    usleep(50);

                    bcm2835_gpio_write(CLOCK_PIN, 0);
                }
            }
     usleep(COLOR_LATCH_DURATION);
}

void clear_leds(void)
{
    unsigned char leds[NUM_LED * 3];

    memset(leds, 0, sizeof(leds));
    set_led_colors(leds);
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
        usleep(100000);

        set_led_colors(leds2);
        usleep(100000);
    }
    clear_leds();
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
                color_hue(i + j, &led);
                leds[(j * 3)] = led.red;
                leds[(j * 3) + 1] = led.green;
                leds[(j * 3) + 2] = led.blue;
            }
            set_led_colors(leds);
            usleep(10000);
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
        usleep(10000);
    }
}

int main(int argc, char *argv[])
{
    setup();
    startup();
    rainbow();
    return 0;
}
