CC=avr-gcc
CFLAGS=-g -Os -Wall -mcall-prologues -mmcu=atmega324a -D F_CPU=8000000UL
OBJ2HEX=avr-objcopy 
TARGET=ledstick-avr

program: $(TARGET).hex 
	sudo avrdude -p m324a -P usb -c avrispmkII -Uflash:w:$(TARGET).hex -B 1.0 

$(TARGET).hex: $(TARGET).obj
	$(OBJ2HEX) -R .eeprom -O ihex $< $@

$(TARGET).obj: $(TARGET).o hue.o 
	$(CC) $(CFLAGS) -o $@ -Wl,-Map,$(TARGET).map $(TARGET).o hue.o -lm

clean:
	rm -f *.hex *.obj *.o *.map
