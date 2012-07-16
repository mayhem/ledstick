#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <bcm2835.h>
#include "hue.h"
#include "gpio.h"
#include "ledstick.h"

// pin definitions
// Power: 5v at P1-02
// Ground:      P1-06
// Data:        P1-
// Clock:       P1-

#define DATA_PIN  8
#define CLOCK_PIN 7  

#define COLOR_LATCH_DURATION 501
#define CLOCK_PERIOD 100

void ledstick_setup(void)
{
    setup_io();

    INP_GPIO(DATA_PIN); // must use INP_GPIO before we can use OUT_GPIO
    OUT_GPIO(DATA_PIN);

    INP_GPIO(CLOCK_PIN); // must use INP_GPIO before we can use OUT_GPIO
    OUT_GPIO(CLOCK_PIN);

    // Reset the LED chain
    GPIO_CLR = 1<<CLOCK_PIN;
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
                    if (byte & (1 << (8 - i)))
                        GPIO_SET = 1 << DATA_PIN;
                    else
                        GPIO_CLR = 1 << DATA_PIN;
                    usleep(CLOCK_PERIOD);

                    GPIO_SET = 1 << CLOCK_PIN;
                    usleep(CLOCK_PERIOD);

                    GPIO_CLR = 1 << CLOCK_PIN;
                }
            }
     usleep(COLOR_LATCH_DURATION);
}
