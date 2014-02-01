# customize the following paths for your computer
CC = "xc8.exe"
LIB_INC_PATH = "./Include"

CHIP = 16F1454

CFLAGS = --chip=$(CHIP) -Q -G  --double=24 --float=24
CFLAGS += --rom=default,-0-FFF,-1FFF-2000
CFLAGS += --codeoffset=0x1000
CFLAGS += --opt=default,+asm,-asmfile,+speed,-space,-debug --addrqual=ignore
CFLAGS += --mode=pro -N64 -I. -I$(LIB_INC_PATH) --warn=0 --asmlist --summary=default,-psect,-class,+mem,-hex,-file --output=default,-inhx032 
CFLAGS += --runtime=default,+clear,+init,-keep,-no_startup,+osccal,-resetbits,-download,-stackcall,+clib

MOUSE_OBJS = usb_device.p1 usb_function_hid.p1 usb_descriptors.p1 main.p1

MOUSE_HDRS = usb_config.h HardwareProfile.h

all: mouse.hex

mouse.hex: $(MOUSE_OBJS)
	$(CC) $(CFLAGS) -o./$@ $(MOUSE_OBJS)

%.p1: %.c $(MOUSE_HDRS) Makefile
	$(CC) --pass1 $(CFLAGS) -o./$@ $<

clean:
#	rm -f mouse.hex
	rm -f *.p1 *.d *.pre *.sym *.cmf *.cof *.hxl *.lst *.obj *.rlf *.sdb
	rm -f funclist
