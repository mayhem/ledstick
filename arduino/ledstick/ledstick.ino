#include <Adafruit_NeoPixel.h>
#include <Timer.h>
#include <avr/pgmspace.h>

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream
//   NEO_GRB     Pixels are wired for GRB bitstream
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip)

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

    for(i = 0; i < 30; i++)
    {
        for(j = 0; j < DEVICE_HEIGHT; j++)
        {
            if (((i/4) + (j/4)) % 2 == 0)
                strip.setPixelColor(j, col1.r, col1.g, col1.b);
            else
                strip.setPixelColor(j, col2.r, col2.g, col2.b);
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
    const prog_uchar *palette;
    
    char buf[32];
    
    width = bitmaps[index].width;
    height = bitmaps[index].height;
    palette = bitmaps[index].palette;
    offset = height * col;
    for(row = 0; row < height; row++)    
    {
        pixel = pgm_read_byte(bitmaps[index].pixels + offset + row);
        red = pgm_read_byte_near(palette + ((uint16_t)pixel * 3));
        green = pgm_read_byte_near(palette + ((uint16_t)pixel * 3) + 1);
        blue = pgm_read_byte_near(palette + ((uint16_t)pixel * 3) + 2);
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
}

void loop() 
{
    static uint16_t col = 0;
    static uint8_t  image = 0, pass = 0;
    
    show_col(image, col);
    col++;
    delay(5);
    if (col == cur_width)
    {
        set_color(0, 0, 0);
        delay(50);
        col = 0;
        pass++;
        if (pass == 5)
        {
           image = (image+1) % num_bitmaps;
           cur_width = bitmaps[image].width;
           pass = 0;
        }
    }    
}

