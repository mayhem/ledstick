#include <Adafruit_NeoPixel.h>
#include <Timer.h>

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream
//   NEO_GRB     Pixels are wired for GRB bitstream
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip)

#include "ledstick.h" 

// bitmap stuff
Adafruit_NeoPixel strip = Adafruit_NeoPixel(MAX_HEIGHT, 2, NEO_GRB + NEO_KHZ800);
bitmap_t *load_bitmap = NULL;
bitmap_t *active_bitmap = NULL;
uint8_t   col = 0;

// Time keeping
Timer t;
uint32_t ticks = 0;
uint32_t target = 0;
uint32_t num_received = 0;

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
    set_color(0, 0, 0);
}

void set_color(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t j;
    
    for(j = 0; j < MAX_HEIGHT; j++)
        strip.setPixelColor(j, r, g, b);
    strip.show();
}

void show_col(bitmap_t &bitmap, int col)
{
    int row;
    char *ptr;
    
    for(row = 0; row < bitmap.h; row++)    
    {
        ptr = (char *)&bitmap.pixels[row * bitmap.w + (col * 3)];
        strip.setPixelColor(bitmap.h - row, *ptr++, *ptr++, *ptr); 
    }
    strip.show(); 
}

uint16_t crc16_update(uint16_t crc, uint8_t a)
{
    int i;

    crc ^= a;
    for (i = 0; i < 8; ++i)
    {
        if (crc & 1)
            crc = (crc >> 1) ^ 0xA001;
        else
            crc = (crc >> 1);
    }

    return crc;
}

int receive_char(char ch, bitmap_t &bitmap)
{
    static int total = 0;
    static char *ptr = 0;
    static uint16_t sent_crc;
    
    // If this is the first character, set pointer to begin of bitmap
    if (num_received == 0)
        ptr = (char *)&bitmap;
        
    //Serial.print("data: ");
    //Serial.print(num_received, DEC);
    //Serial.print(" ");  
    //Serial.println(ch, HEX);       
        
    // store the character    
    *ptr = ch;
    ptr++;
    num_received++;    
 
    if (num_received == 4) //sizeof(bitmap.len))
    {
       Serial.print("bitmap.len ");
       Serial.println(bitmap.len, DEC);
       if (bitmap.len > MAX_PACKET_PAYLOAD)
       {
           num_received = 0;
           ptr = NULL;
           return RECEIVE_ABORT_PACKET;
       }
       
       // total number of bytes to receive, including the crc checksum
       total = sizeof(uint16_t) + bitmap.len + sizeof(uint16_t) + sizeof(uint16_t);
    }   
    if (num_received == total - 1)
       *((char *)&sent_crc) = ch;
    if (num_received == total)
    {
       uint16_t  crc = 0;
       char     *crc_ptr;
       int i;
       
       *(((char *)&sent_crc)+1) = ch;
       num_received = 0;
       ptr = NULL;
       
       for(i = 0, crc_ptr = (char *)&bitmap.w; i < bitmap.len; i++, crc_ptr++)
           crc = crc16_update(crc, *crc_ptr);
       
       Serial.print("sent crc ");
       Serial.println(sent_crc, HEX);
       Serial.print("     crc ");
       Serial.println(crc, HEX);
           
       if (crc != sent_crc)
           return RECEIVE_ABORT_PACKET_CRC;
       
       return RECEIVE_PACKET_COMPLETE;
    } 
  
    return RECEIVE_OK;
}

void setup() 
{
  uint8_t j;
  Serial.begin(57600);
  
  strip.begin();
  
  t.every(1000, tick);
  
  startup();
}

#define HEADER_LEN 4
const char header[HEADER_LEN] = { 0xF0, 0x0F, 0x0F, 0xF0 };
void loop() 
{
    static bitmap_t bitmap0, bitmap1;
    static int count = 0;
    static int total = -1;
    static int header_count = 0;
    static uint32_t timeout = 0;
    char ch;
    int ret;
    
    if (load_bitmap == NULL)
    {
        load_bitmap = &bitmap0;
        active_bitmap = &bitmap1;
    }
    while (Serial.available() > 0) 
    {
        ch = Serial.read();
        if (ch < 0)
            break;
            
        if (header_count < HEADER_LEN)
        {
            Serial.print(" hdr: ");
            Serial.print(header_count, DEC);
            Serial.print(" ");  
            Serial.println(ch, HEX); 
            if (ch == header[header_count])
            {
                header_count++;
                if (header_count == HEADER_LEN)
                    set_color(0, 0, 0);
                continue;
            }
            header_count = 0;
            continue;
        }
        if (header_count == HEADER_LEN)
            timeout = ticks + 1000;
            
        if (timeout && timeout < ticks)
        {
            timeout = 0;
            num_received = 0;
            Serial.println("timeout!");
            continue;
        }
          
        ret = receive_char(ch, bitmap0);
        if (ret == RECEIVE_ABORT_PACKET)
        {
            set_color(255, 0, 0);
            header_count = 0;
            continue;
        }
        if (ret == RECEIVE_ABORT_PACKET_CRC)
        {
            set_color(0, 0, 255);
            header_count = 0;
            continue;
        }
        if (ret == RECEIVE_PACKET_COMPLETE)
        {
            // use bitmap & swap in bitmap here
            set_color(0, 255, 0);
            header_count = 0;
        }   
    }
    return;
    
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

