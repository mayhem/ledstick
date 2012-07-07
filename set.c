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

    if (argc < 4)
    {
        printf("Usage: setcolor <r> <g> <b>\n");
        return -1;
    }
 
    ledstick_setup();
    for(j = 0; j < NUM_LED; j++)
    {
        leds[(j * 3)] = atoi(argv[1]);
        leds[(j * 3) + 1] = atoi(argv[2]);
        leds[(j * 3) + 2] = atoi(argv[3]);
    }
    set_led_colors(leds);
    return 0;
}
