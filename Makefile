DEBUG	= -g -O0
#DEBUG	= -O3
CC	= gcc
INCLUDE	= -I/usr/local/include
CFLAGS	= $(DEBUG) -Wall $(INCLUDE) -Winline -pipe

LDFLAGS	= -L/usr/local/lib
LIBS    = -lrt

all:	ledstick-ani ledstick-set ledstick-clear
clean:  
	rm -f *.o ledstick-ani ledstick

ledstick-ani: animate.o ledstick.o hue.o gpio.o 
	$(CC) -o $@ animate.o ledstick.o hue.o gpio.o $(LDFLAGS) $(LIBS)

ledstick-clear: clear.o ledstick.o hue.o gpio.o
	$(CC) -o $@ clear.o ledstick.o hue.o gpio.o $(LDFLAGS) $(LIBS)
	
ledstick-set: set.o ledstick.o hue.o gpio.o
	$(CC) -o $@ set.o ledstick.o hue.o gpio.o $(LDFLAGS) $(LIBS)
