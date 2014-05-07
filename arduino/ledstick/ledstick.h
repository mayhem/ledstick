#define MAX_WIDTH  110
#define MAX_HEIGHT 144

// BITMAP header does NOT include the 2 byte size!
#define BITMAP_HEADER_SIZE (sizeof(uint32_t) + (sizeof(uint16_t) * 3))
#define MAX_PACKET_PAYLOAD (BITMAP_HEADER_SIZE + (MAX_WIDTH * MAX_HEIGHT * 3)) 

#define HEADER_LEN 4

#define RECEIVE_OK               0
#define RECEIVE_ABORT_PACKET     1
#define RECEIVE_ABORT_PACKET_CRC 2
#define RECEIVE_PACKET_COMPLETE  3

struct color_t
{
  uint8_t   r,g,b;
};

struct bitmap_t
{
  uint32_t  len;
  uint16_t  w, h;
  uint16_t  col_delay;
  color_t   pixels[MAX_HEIGHT * MAX_WIDTH];
};


