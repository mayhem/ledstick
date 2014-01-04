#include <Adafruit_NeoPixel.h>
#include <Timer.h>

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream
//   NEO_GRB     Pixels are wired for GRB bitstream
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip)

#define MAX_WIDTH  5
#define MAX_HEIGHT 5

struct color_t
{
  uint8_t   r,g,b;
};

#define BITMAP_HEADER_SIZE 4
struct bitmap_t
{
  uint8_t   w, h;
  uint16_t  col_delay;
  color_t   pixels[MAX_HEIGHT * MAX_WIDTH];
};  

// bitmap stuff
Adafruit_NeoPixel strip = Adafruit_NeoPixel(MAX_HEIGHT, 2, NEO_GRB + NEO_KHZ800);
bitmap_t  bitmap0, bitmap1;
bitmap_t *load_bitmap, *active_bitmap;
uint8_t   col = 0;

// Time keeping
Timer t;
uint32_t ticks = 0;
uint32_t target = 0;

// input byte counters
int32_t count = 0;
int32_t total = -1;

void tick()
{
   ticks++;
}

void startup(void)
{
    uint8_t i, j;
    color_t col1 = { 255, 140, 0 };
    color_t col2 = { 255, 0, 255 };

    for(i = 0; i < 5; i++)
    {
        for(j = 0; j < MAX_HEIGHT; j++)
        {
            if ((i + j) % 2 == 0)
                strip.setPixelColor(j, col1.r, col1.g, col1.b);
            else
                strip.setPixelColor(j, col2.r, col2.g, col2.b);
        }   
        strip.show();
        delay(250);
    }
    for(j = 0; j < MAX_HEIGHT; j++)
        strip.setPixelColor(j, 0, 0, 0);
    strip.show();
}

void set_color(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t j;
    
    for(j = 0; j < MAX_HEIGHT; j++)
        strip.setPixelColor(j, r, g, b);
    strip.show();
    delay(500);
}

void setup() 
{
  uint8_t j;
  Serial.begin(115200);
  
  strip.begin();
  
  load_bitmap = &bitmap0;
  active_bitmap = &bitmap1;
  
  t.every(1000, tick);
  
  startup();
}

void loop() 
{
    while (Serial.available()) 
    {
        uint8_t *bitmap = (uint8_t *) &load_bitmap;
        bitmap[count] = Serial.read();
        count++;

        if (count == 2)
        {
          set_color(255, 0, 0);
          total = load_bitmap->w * load_bitmap->h + BITMAP_HEADER_SIZE;
          continue;
        }
        if (count == total)
        {

            bitmap_t *temp = load_bitmap;
            load_bitmap = active_bitmap;
            active_bitmap = temp;
            count = 0;
            total = 0;
            target = 0;
        }
    }
    
    if (target == 0)
        target = ticks;
        
    if (ticks >= target)
    {
        uint8_t *ptr, row;
        
        strip.show();
        target += active_bitmap->col_delay;
        
        for(row = 0; row < active_bitmap->h; row++)
        {
            ptr = (uint8_t *)&active_bitmap->pixels[row * active_bitmap->w + (col * 3)];
            strip.setPixelColor(row, *ptr++, *ptr++, *ptr); 
        }
        
        col = (col + 1) % active_bitmap->w;
    }            
}

