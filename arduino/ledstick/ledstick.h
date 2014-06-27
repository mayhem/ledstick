#define MAX_WIDTH  100
#define MAX_HEIGHT 144

// BITMAP header does NOT include the 2 byte size!
#define BITMAP_HEADER_SIZE (sizeof(uint32_t) + (sizeof(uint16_t) * 3))
#define MAX_PACKET_PAYLOAD (BITMAP_HEADER_SIZE + (MAX_WIDTH * MAX_HEIGHT * sizeof(color_t))) 

#define HEADER_LEN 4
#define RECEIVE_OK               0
#define RECEIVE_ABORT_PACKET     1
#define RECEIVE_ABORT_PACKET_CRC 2
#define RECEIVE_PACKET_COMPLETE  3
#define RECEIVE_NO_STATUS        4
#define RECEIVE_TIMEOUT          5

#define PACKET_OFF               0
#define PACKET_SHOW_IMAGE_0      1
#define PACKET_SHOW_IMAGE_1      2
#define PACKET_LOAD_IMAGE_0      3
#define PACKET_LOAD_IMAGE_1      4
#define PACKET_IMAGE_BLOB        5

#define BYTES_PER_BLOB           10240

struct color_t
{
  uint8_t   r,g,b;
};

struct packet_t
{
  uint8_t   type;
};

struct bitmap_t
{
  uint16_t  w, h;
  uint16_t  col_delay;
  color_t   pixels[MAX_HEIGHT * MAX_WIDTH];
};


