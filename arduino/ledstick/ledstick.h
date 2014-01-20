#define MAX_WIDTH  72
#define MAX_HEIGHT 72

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
  uint16_t     width, height;
  uint16_t     col_delay;
  const prog_uchar  *pixels;
  const prog_uchar  *palette;
}; 

