
#include <Adafruit_NeoPixel.h>
#include <Timer.h>
#include <avr/pgmspace.h>

#include "ledstick.h" 
#include "bitmaps.h"

#define DEVICE_HEIGHT 144

// led stuff
Adafruit_NeoPixel strip = Adafruit_NeoPixel(DEVICE_HEIGHT, 2, NEO_GRB + NEO_KHZ800);
const int led = 13;

// Time keeping
Timer t;
uint32_t ticks = 0;
uint32_t target = 0;

// Communication stuff
uint8_t  io_blob_mode = 0; 
int io_total_bytes = 0;
int io_header_count = 0;
uint32_t io_timeout = 0;
int io_num_received = 0;
const char io_header[HEADER_LEN] = { 0xF0, 0x0F, 0x0F, 0xF0 };
uint8_t io_response = RECEIVE_NO_STATUS;

// bitmap related stuff -- note this also includes static bitmaps from bitmaps.h!
const int num_bitmaps = 2;
bitmap_t bitmaps[num_bitmaps];

// slicer
uint8_t slicer_show_image = 1;
uint8_t slicer_image_index = 0;
uint8_t slicer_image_static = 1;
uint8_t slicer_col_index = 0;
uint8_t slicer_pass = 0;

void slicer_setup(void)
{
    // clear the bitmaps and set widths to 0, which indicates its not loaded
    memset(bitmaps, 0, sizeof(bitmaps));
}
void slicer_select_image(uint8_t index, uint8_t stat)
{
    slicer_show_on(0);
    slicer_image_index = index;
    slicer_image_static = stat;
    slicer_pass = 0;
    slicer_show_on(1);
}

void slicer_show_on(uint8_t state)
{
    slicer_show_image = state;
    if (!state)
    {
        slicer_col_index = 0;
        show_color(20, 10, 80);
    }
}

uint8_t slicer_is_on(void)
{
    return slicer_show_image;
}

void slicer_next_image(void)
{
     for(;;)
     {
         slicer_image_index++;
         if (slicer_image_static)
         {         
             if (slicer_image_index == num_static_bitmaps)
             {
                 slicer_image_index = 0;
                 slicer_image_static = 0;
             }
         }
         else
         {
             if (slicer_image_index == num_bitmaps)
             {
                 slicer_image_index = 0;
                 slicer_image_static = 1;
             }
         }
         if (!slicer_image_static && bitmaps[slicer_image_index].w == 0)
             continue;    
         break;
     }
}

uint8_t slicer_get_cur_width(void)
{
    return slicer_image_static ? static_bitmaps[slicer_image_index].w : bitmaps[slicer_image_index].w; 
}

void slicer_loop(void)
{  
    if (!slicer_show_image)
        return;
        
    _slicer_show_col(slicer_image_index, slicer_col_index, slicer_image_static);
    slicer_col_index++;
    delayMicroseconds(200);
    if (slicer_col_index == slicer_get_cur_width())
    {
        show_color(0, 0, 0);
        delay(50);
        slicer_col_index = 0;
        slicer_pass++;
        if (slicer_pass == 20)
        {
           slicer_next_image();
           slicer_pass = 0;
        }
    }   
}

void _slicer_show_col(uint8_t index, uint16_t col, uint8_t stat)
{
    uint16_t row, offset;
    uint16_t width, height;
    uint8_t pixel, red, green, blue;
    
    if (stat)
    {
        width = static_bitmaps[index].w;
        height = static_bitmaps[index].h;
    }    
    else
    {
        width = bitmaps[index].w;
        height = bitmaps[index].h;
    }  
    
    offset = height * 3 * col;
    for(row = 0; row < height; row++)    
    {
        if (stat)
        {
            red = pgm_read_byte_near(static_bitmaps[index].pixels + offset + (row * 3));
            green = pgm_read_byte_near(static_bitmaps[index].pixels + offset + (row * 3) + 1);
            blue = pgm_read_byte_near(static_bitmaps[index].pixels + offset + (row * 3) + 2);
        }
        else
        {
            red = *((char *)bitmaps[index].pixels + offset + (row * 3));
            green = *((char *)bitmaps[index].pixels + offset + (row * 3) + 1);
            blue = *((char *)bitmaps[index].pixels + offset + (row * 3) + 2);
        }
        strip.setPixelColor(height - row - 1, red / 3, green / 3, blue / 3); 
    }
    strip.show(); 
}

uint16_t io_crc16_update(uint16_t crc, uint8_t a)
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

int io_receive_char(char ch, char *packet, uint16_t *len)
{
    static char *ptr = 0;
    static uint16_t sent_crc;
    
    // If this is the first character, set pointer to begin of bitmap
    if (io_num_received == 0)
        ptr = (char *)len;
    else    
    if (io_num_received == sizeof(uint16_t))
        ptr = packet;
    else
    if (io_num_received > sizeof(uint16_t) && io_num_received == *len + 2)
        ptr = (char *)&sent_crc;
        
    // store the character    
    *ptr = ch;
    ptr++;
    io_num_received++;    
 
    if (io_num_received == sizeof(uint16_t))
    {
       if (*len > MAX_PACKET_PAYLOAD)
       {
           io_num_received = 0;
           ptr = NULL;
           return RECEIVE_ABORT_PACKET;
       }
       
       slicer_show_image = 0;
       // total number of bytes to receive, including packet and size the crc checksum
       io_total_bytes = *len + sizeof(uint16_t) + sizeof(uint16_t);
       io_response = RECEIVE_NO_STATUS;
    }   
       
    if (io_total_bytes > 0 && io_num_received == io_total_bytes)
    {
       uint16_t  crc = 0;
       char     *crc_ptr;
       int i;
       
       io_num_received = 0;
       ptr = NULL;
       
       for(i = 0, crc_ptr = packet; i < *len; i++, crc_ptr++)
           crc = io_crc16_update(crc, *crc_ptr);     

       if (crc != sent_crc)
           return RECEIVE_ABORT_PACKET_CRC;

       return RECEIVE_PACKET_COMPLETE;
    } 
  
    return RECEIVE_OK;
}

void io_reset_receive()
{
    io_timeout = 0;
    io_header_count = 0;
    io_num_received = 0;
    io_total_bytes = 0;
}

void serialEvent1()
{
    int                ret;
    char               ch;
    static   packet_t  packet;
    static   uint16_t  len = 0, blob_offset = 0, total_len = 0;
    static   bitmap_t *dest_bitmap = NULL;
    
    while (Serial1.available()) 
    {
        if (slicer_is_on())
            slicer_show_on(0);
            
        ch = Serial1.read(); 
        if (io_header_count < HEADER_LEN)
        {
            if (ch == io_header[io_header_count])
            {
                io_header_count++;
                continue;
            }
            io_header_count = 0;
            continue;
        }
        if (io_header_count == HEADER_LEN)
        {
            //clear the io_timeout led
            digitalWrite(led, LOW);
            io_timeout = ticks + 2;
            
            // so that this if doesn't fire again
            io_header_count++;
        }
        
        if (!io_blob_mode)
        {
            io_response = io_receive_char(ch, (char *)&packet, &len);
            if (io_response == RECEIVE_OK)
                continue;
                
            Serial1.write(io_response);
            if (io_response == RECEIVE_PACKET_COMPLETE)
            {
                if (packet.type == PACKET_LOAD_IMAGE_0 || packet.type == PACKET_LOAD_IMAGE_1)
                {
                     dest_bitmap = packet.type == PACKET_LOAD_IMAGE_0 ? &bitmaps[0] : &bitmaps[1];
                     io_blob_mode = 1;
                     len = 0;
                     io_header_count = 0;
                     blob_offset = 0;
                     total_len = 0;
                } 
            }
            io_reset_receive();         
        }
        else
        {
            io_response = io_receive_char(ch, (char *)dest_bitmap + blob_offset, &len);
            if (io_response == RECEIVE_OK)
                continue;
        
            if (io_response == RECEIVE_PACKET_COMPLETE &&  
               (packet.type == PACKET_LOAD_IMAGE_0 || packet.type == PACKET_LOAD_IMAGE_1))
            { 
                 
                 int total = dest_bitmap->w * dest_bitmap->h * 3;
                 int percent = 100 * (blob_offset + len) / total;
                 show_percent_complete(percent);
            }  
               
            Serial1.write(io_response);
            if (io_response == RECEIVE_PACKET_COMPLETE)
            {           
                if (packet.type == PACKET_LOAD_IMAGE_0 || packet.type == PACKET_LOAD_IMAGE_1)
                {
                     total_len += len;
                     if (len == BYTES_PER_BLOB)
                     {
                         blob_offset += len;
                         len = 0;
                         io_header_count = 0;
                     }
                     else
                     {
                         
                         dest_bitmap = NULL;
                         io_blob_mode = 0;
                         len = 0;
                         io_header_count = 0;

                         if (packet.type == PACKET_LOAD_IMAGE_0)
                             slicer_select_image(0, 0);
                         else
                             slicer_select_image(1, 0);
                     }
                } 
            }
            io_reset_receive();         
        }        
    }
    return;
}

void startup_animation(void)
{
    uint8_t i, j;
    color_t col1 = { 128, 70, 0 };
    color_t col2 = { 128, 0, 128 };

    for(i = 0; i < 10; i++)
    {       
        for(j = 0; j < DEVICE_HEIGHT; j++)
        {
            if (i % 2 == 0)
            {
                digitalWrite(led, HIGH);
                if (j % 2 == 1)
                    strip.setPixelColor(j, col1.r, col1.g, col1.b);
                else
                    strip.setPixelColor(j, 0, 0, 0);    
            }
            else
            {
                digitalWrite(led, LOW);
                if (j % 2 == 0)
                    strip.setPixelColor(j, col2.r, col2.g, col2.b);
                else
                    strip.setPixelColor(j, 0, 0, 0);  
            }
        }   
        strip.show();
        delay(100);
    }
    show_color(0, 0, 0);
}

void show_color(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t j;
    
    for(j = 0; j < DEVICE_HEIGHT; j++)
        strip.setPixelColor(j, r, g, b);
    strip.show();
}

void show_percent_complete(uint8_t percent)
{
    uint8_t i;
    int threshold;
    
    threshold = (int)percent * DEVICE_HEIGHT / 100;
    for(i = 0; i < DEVICE_HEIGHT; i++)
    {
        if (i >= threshold)
            strip.setPixelColor(i, 20, 10, 80);
        else
            strip.setPixelColor(i, 10, 80, 20);    
    }
    strip.show();
}

void show_sparkle_animation(void)
{
    uint8_t i, color_index = 0;
    
    for(;;)
    {
      for(i = 0; i < DEVICE_HEIGHT; i++)
          strip.setPixelColor(i, 0,0,0);
      for(i = 0; i < 8; i++)
          strip.setPixelColor(random(DEVICE_HEIGHT), color_wheel(color_index));
      color_index++;
      strip.show();
      delay(50);
    }
}

uint32_t color_wheel(byte wheel_pos) 
{
    if(wheel_pos < 85) 
    {
       return strip.Color(wheel_pos * 3, 255 - wheel_pos * 3, 0);
    } 
    else 
    if (wheel_pos < 170) 
    {
       wheel_pos -= 85;
       return strip.Color(255 - wheel_pos * 3, 0, wheel_pos * 3);
    } 
    else 
    {
       wheel_pos -= 170;
       return strip.Color(0, wheel_pos * 3, 255 - wheel_pos * 3);
    }
}

void tick()
{
    ticks++;
}

void setup() 
{  
    pinMode(led, OUTPUT);
    
    Serial.begin(115200);
    Serial1.begin(115200);
    
    strip.begin();
    
    t.every(1000, tick);

    Serial.println("\ndas blinkenstick!");
    startup_animation();
    //show_sparkle_animation();
}

void loop()
{    
    t.update();
    if (io_timeout && io_timeout < ticks)
    {
        Serial.print("io_timeout");
        io_reset_receive();
        io_blob_mode = 0;
        io_response = RECEIVE_TIMEOUT;
        slicer_select_image(0, 1);
    }
  
    slicer_loop();   
}

