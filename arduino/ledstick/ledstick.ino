
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

// Communication stuff

const int num_bitmaps = 2;
bitmap_t bitmaps[num_bitmaps];
uint8_t  blob_mode = 0; 
int total_bytes = 0;
int header_count = 0;
uint32_t timeout = 0;
int num_received = 0;
const char header[HEADER_LEN] = { 0xF0, 0x0F, 0x0F, 0xF0 };
uint8_t response = RECEIVE_NO_STATUS;
int show_image = 0;
const int led = 13;

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
            {
                
                if (j % 2 == 1)
                    strip.setPixelColor(j, col1.r, col1.g, col1.b);
                else
                    strip.setPixelColor(j, 0, 0, 0);    
            }
            else
            {
                
                if (j % 2 == 0)
                    strip.setPixelColor(j, col2.r, col2.g, col2.b);
                else
                    strip.setPixelColor(j, 0, 0, 0);  
            }
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

void show_static_col(uint8_t index, uint16_t col)
{
    uint16_t row, offset;
    uint16_t width, height;
    uint8_t pixel, red, green, blue;
    
    width = static_bitmaps[index].w;
    height = static_bitmaps[index].h;
    offset = height * 3 * col;
    for(row = 0; row < height; row++)    
    {
        red = pgm_read_byte_near(static_bitmaps[index].pixels + offset + (row * 3));
        green = pgm_read_byte_near(static_bitmaps[index].pixels + offset + (row * 3) + 1);
        blue = pgm_read_byte_near(static_bitmaps[index].pixels + offset + (row * 3) + 2);
        strip.setPixelColor(height - row - 1, red / 3, green / 3, blue / 3); 
    }
    strip.show(); 
}

int skip = 0;
void show_col(uint8_t index, uint16_t col)
{
    uint16_t row, offset;
    uint16_t width, height;
    uint8_t pixel, red, green, blue;
    
    width = bitmaps[index].w;
    height = bitmaps[index].h;
    offset = height * 3 * col;
    for(row = 0; row < height; row++)    
    {
        red = *((char *)bitmaps[index].pixels + offset + (row * 3));
        green = *((char *)bitmaps[index].pixels + offset + (row * 3) + 1);
        blue = *((char *)bitmaps[index].pixels + offset + (row * 3) + 2);
        
        strip.setPixelColor(height - row - 1, red / 3, green / 3, blue / 3); 
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

int receive_char(char ch, char *packet, uint16_t *len)
{
    static char *ptr = 0;
    static uint16_t sent_crc;
    
    // If this is the first character, set pointer to begin of bitmap
    if (num_received == 0)
        ptr = (char *)len;
    else    
    if (num_received == sizeof(uint16_t))
    {
        ptr = packet;
        Serial.println((long int)ptr, HEX);
    }
    else
    if (num_received > sizeof(uint16_t) && num_received == *len + 2)
        ptr = (char *)&sent_crc;
        
    // store the character    
    *ptr = ch;
    ptr++;
    num_received++;    
 
    if (num_received == sizeof(uint16_t))
    {
       if (*len > MAX_PACKET_PAYLOAD)
       {
           num_received = 0;
           ptr = NULL;
           Serial1.write(RECEIVE_ABORT_PACKET);
           return RECEIVE_ABORT_PACKET;
       }
       
       show_image = 0;
       // total number of bytes to receive, including packet and size the crc checksum
       total_bytes = *len + sizeof(uint16_t) + sizeof(uint16_t);
       response = RECEIVE_NO_STATUS;
    }   
       
    if (total_bytes > 0 && num_received == total_bytes)
    {
       uint16_t  crc = 0;
       char     *crc_ptr;
       int i;
       
       num_received = 0;
       ptr = NULL;
       
       for(i = 0, crc_ptr = packet; i < *len; i++, crc_ptr++)
           crc = crc16_update(crc, *crc_ptr);     

       if (crc != sent_crc)
       {
           Serial1.write(RECEIVE_ABORT_PACKET_CRC);
           return RECEIVE_ABORT_PACKET_CRC;
       }    

       Serial1.write(RECEIVE_PACKET_COMPLETE);
       return RECEIVE_PACKET_COMPLETE;
    } 
  
    return RECEIVE_OK;
}

void reset_receive()
{
    timeout = 0;
    header_count = 0;
    num_received = 0;
    total_bytes = 0;
}

void dumpImage(uint16_t len, char*image)
{
    uint16_t i;
    
    for(i = 0; i < len; i++)
    {
         if (i % 8 == 0)
         {
              Serial.println("");
              Serial.print(i, HEX);
              Serial.print(" ");
         }
         Serial.print(image[i], HEX);   
         Serial.print(" ");  
    }
    Serial.println("");
    Serial.println(len);
}

void serialEvent1()
{
    int               ret;
    char              ch;
    static   packet_t packet;
    static   uint16_t len = 0, blob_offset = 0, total_len = 0;
    
    while (Serial1.available()) 
    {
        ch = Serial1.read(); 
        if (header_count < HEADER_LEN)
        {
            if (ch == header[header_count])
            {
                header_count++;
                continue;
            }
            header_count = 0;
            continue;
        }
        if (header_count == HEADER_LEN)
        {
            //clear the timeout led
            digitalWrite(led, LOW);
            timeout = ticks + 2;
            
            // so that this if doesn't fire again
            header_count++;
        }
        
        if (!blob_mode)
        {
            response = receive_char(ch, (char *)&packet, &len);
            if (response == RECEIVE_OK)
                continue;
        
            if (response == RECEIVE_PACKET_COMPLETE)
            {
                if (packet.type == PACKET_LOAD_IMAGE_0 || packet.type == PACKET_LOAD_IMAGE_1)
                {
                     blob_mode = 1;
                     len = 0;
                     header_count = 0;
                     blob_offset = 0;
                     total_len = 0;
                } 
            }
            reset_receive();         
        }
        else
        {
            response = receive_char(ch, ((char *)&bitmaps[0]) + blob_offset, &len);
            if (response == RECEIVE_OK)
                continue;
        
            if (response == RECEIVE_PACKET_COMPLETE)
            {
                if (packet.type == PACKET_LOAD_IMAGE_0 || packet.type == PACKET_LOAD_IMAGE_1)
                {
                     total_len += len;
                     if (len == BYTES_PER_BLOB)
                     {
                         blob_offset += len;
                         len = 0;
                         header_count = 0;
                     }
                     else
                     {
                         blob_mode = 0;
                         len = 0;
                         header_count = 0;
                         cur_width = bitmaps[0].w;
                         show_image = 1;

                     }
                } 
            }
            reset_receive();         
        }        
    }
    return;
}

void setup() 
{ 
    int i;
    
    pinMode(led, OUTPUT);
    
    for(i = 0; i < 10; i++)
    {
        if (i % 2 == 0)
            digitalWrite(led, HIGH);
        else
            digitalWrite(led, LOW);
        delay(100);
    }
  
    Serial.begin(115200);
    Serial1.begin(115200);
    
    strip.begin();

    startup();
    
    cur_width = bitmaps[0].w;
    t.every(1000, tick);
    
    Serial.println("ledstick!");
}

void loop()
{    
    static uint16_t col = 0;
    static uint8_t  image = 0, pass = 0;

    t.update();
    if (timeout && timeout < ticks)
    {
        Serial.print("timeout -- received: ");
        Serial.println(num_received, DEC);
        digitalWrite(led, HIGH);
        reset_receive();
        blob_mode = 0;
        response = RECEIVE_TIMEOUT;
    }  

    if (!show_image)
        return;
        
    show_col(image, col);
    col++;
    delayMicroseconds(200);
    if (col == cur_width)
    {
        set_color(0, 0, 0);
        delay(50);
        col = 0;
        //pass++;
        if (pass == 20)
        {
           image = (image+1) % num_bitmaps;
           cur_width = bitmaps[image].w;
           pass = 0;
        }
    }    
}

