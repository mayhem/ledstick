#ifndef __LEDSTICK_H__
#define __LEDSTICK_H__

#define NUM_LED              4

void ledstick_setup(void);
void set_led_colors(unsigned char leds[NUM_LED * 3]);

#endif
