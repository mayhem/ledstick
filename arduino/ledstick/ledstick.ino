#include <Adafruit_NeoPixel.h>
#include <Timer.h>
#include <avr/pgmspace.h>

#include "ledstick.h" 
#include "bitmaps.h"

#define DEVICE_HEIGHT 144

// bitmap stuff
Adafruit_NeoPixel strip = Adafruit_NeoPixel(DEVICE_HEIGHT, 2, NEO_GRB + NEO_KHZ800);


// Time keeping
Timer t;
uint32_t ticks = 0;
uint32_t target = 0;
uint16_t cur_width = 0;

void tick()
{
   ticks++;
}

void startup(void)
{
    uint8_t i, j;
    color_t col1 = { 128, 70, 0 };
    color_t col2 = { 128, 0, 128 };

    for(i = 0; i < 10; i++)
    {
        for(j = 0; j < DEVICE_HEIGHT; j++)
        {
            if (i % 2 == 0)
                if (j % 2 == 1)
                    strip.setPixelColor(j, col1.r, col1.g, col1.b);
                else
                    strip.setPixelColor(j, 0, 0, 0);    
            else
                if (j % 2 == 0)
                    strip.setPixelColor(j, col2.r, col2.g, col2.b);
                else
                    strip.setPixelColor(j, 0, 0, 0);  
        }   
        strip.show();
        delay(100);
    }
    set_color(0, 0, 0);
}

void set_color(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t j;
    
    for(j = 0; j < DEVICE_HEIGHT; j++)
        strip.setPixelColor(j, r, g, b);
    strip.show();
}

void show_col(uint8_t index, uint16_t col)
{
    uint16_t row, offset;
    uint16_t width, height;
    uint8_t pixel, red, green, blue;
    
    width = bitmaps[index].width;
    height = bitmaps[index].height;
    offset = height * 3 * col;
    for(row = 0; row < height; row++)    
    {
        red = pgm_read_byte_near(bitmaps[index].pixels + offset + (row * 3));
        green = pgm_read_byte_near(bitmaps[index].pixels + offset + (row * 3) + 1);
        blue = pgm_read_byte_near(bitmaps[index].pixels + offset + (row * 3) + 2);
        strip.setPixelColor(height - row, red / 3, green / 3, blue / 3); 
    }
    strip.show(); 
}

void setup() 
{
  uint8_t j;
  Serial.begin(19200);
  
  strip.begin();
  
  t.every(1000, tick);
  
  startup();
  cur_width = bitmaps[0].width;
  Serial.write("ledstick ready\n");
}

void loop() 
{
    static uint16_t col = 0;
    static uint8_t  image = 0, pass = 0;
    
    show_col(image, col);
    col++;
    delayMicroseconds(200);
    if (col == cur_width)
    {
        set_color(0, 0, 0);
        delay(50);
        col = 0;
        pass++;
        if (pass == 20)
        {
           image = (image+1) % num_bitmaps;
           cur_width = bitmaps[image].width;
           pass = 0;
        }
    }    
}

