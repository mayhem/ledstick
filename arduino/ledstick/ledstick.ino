
#include <Adafruit_NeoPixel.h>
#include <Timer.h>
#include <avr/pgmspace.h>



#include "ledstick.h" 

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
int count = 0;
int total = -1;
int header_count = 0;
uint32_t timeout = 0;
int num_received = 0;
const char header[HEADER_LEN] = { 0xF0, 0x0F, 0x0F, 0xF0 };

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
        red = pgm_read_byte_near(bitmaps[index].pixels + offset + (row * 3));
        green = pgm_read_byte_near(bitmaps[index].pixels + offset + (row * 3) + 1);
        blue = pgm_read_byte_near(bitmaps[index].pixels + offset + (row * 3) + 2);
        strip.setPixelColor(height - row, red / 3, green / 3, blue / 3); 
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
           Serial.write("0x02\n");
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
       Serial.print("received: ");
       Serial.println(num_received, DEC);
           
       if (crc != sent_crc)
       {
           Serial.write("0x00\n");
           return RECEIVE_ABORT_PACKET_CRC;
       }
       
       Serial.write("0x01\n");
       return RECEIVE_PACKET_COMPLETE;
    } 
  
    return RECEIVE_OK;
}

void serialEvent()
{
    int ch, ret;
    
    while (Serial.available() > 0) 
    {
        ch = Serial.read();
        if (ch < 0)
            break;
            
        if (header_count < HEADER_LEN)
        {
            //Serial.print(" hdr: ");
            //Serial.print(header_count, DEC);
            //Serial.print(" ");  
            //Serial.println(ch, HEX); 
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
            timeout = ticks + 2;
                      

        ret = receive_char(ch, bitmaps[0]);
        //ret = RECEIVE_OK;
        //num_received++;
        if (ret != RECEIVE_OK)
            timeout = 0;
            
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
            return;
        }   
    }
    return;
}

const int led = 13;
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
    
    strip.begin();

    startup();
    
    cur_width = bitmaps[0].w;
    t.every(1000, tick);
    
    Serial.print("max headroom: ");
    Serial.println(MAX_PACKET_PAYLOAD, DEC);
}

void loop()
{    
    if (timeout && timeout < ticks)
    {
        Serial.print("timeout -- received: ");
        Serial.println(num_received, DEC);
        timeout = 0;
        num_received = 0;

    }  
    t.update();
}

#if 0
void _loop() 
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
           cur_width = bitmaps[image].w;
           pass = 0;
        }
    }    
}
#endif

