#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include "ledstick.h"

int main(int argc, char *argv[])
{
    int     i, j;
    unsigned char leds[NUM_LED * 3];

    ledstick_setup();
    memset(leds, 0, sizeof(leds));
    set_led_colors(leds);
    return 0;
}
