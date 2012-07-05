DEBUG	= -g -O0
#DEBUG	= -O3
CC	= gcc
INCLUDE	= -I/usr/local/include
CFLAGS	= $(DEBUG) -Wall $(INCLUDE) -Winline -pipe

LDFLAGS	= -L/usr/local/lib
LIBS    = -lbcm2835 -lpthread -lrt

SRC	= ledstick.c hue.c

all:	ledstick
clean:  
	rm -f *.o ledstick

ledstick: ledstick.o hue.o
	$(CC) -o $@ ledstick.o $(LDFLAGS) $(LIBS)
	
