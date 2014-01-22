# customize the following paths for your computer
CC = "xc8.exe"
LIB_INC_PATH = "./Include"

# customize for your part; likely choices are: 16F1454, 16F1455, or 16F1459
CHIP = 16F1454

#uncomment the following to customize
#ADD_CDC = y

CFLAGS = --chip=$(CHIP) -Q -G  --double=24 --float=24
CFLAGS += --rom=default,-1000-1FFF 
CFLAGS += --opt=default,+asm,-asmfile,+speed,-space,-debug --addrqual=ignore
CFLAGS += --mode=pro -N64 -I. -I$(LIB_INC_PATH) --asmlist --summary=default,-psect,-class,+mem,-hex,-file --output=default,-inhx032 
CFLAGS += --runtime=default,+clear,+init,-keep,-no_startup,+osccal,-resetbits,-download,-stackcall,+clib

ifdef ADD_CDC
CFLAGS += -DADD_CDC
endif

HIDPLUS_OBJS = usb_device.p1 usb_function_hid.p1 hidplus.p1
ifdef ADD_CDC
HIDPLUS_OBJS += madness.p1 usb_function_cdc.p1
endif
HIDPLUS_OBJS += cdc_descriptors.p1
HIDPLUS_HDRS = usb_config.h HardwareProfile.h

all: hidplus.hex

hidplus.hex: $(HIDPLUS_OBJS)
	$(CC) $(CFLAGS) -o./$@ $(HIDPLUS_OBJS)

%.p1: %.c $(HIDPLUS_HDRS) Makefile
	$(CC) --pass1 $(CFLAGS) -o./$@ $<

clean:
#	rm -f hidplus.hex
	rm -f *.p1 *.d *.pre *.sym *.cmf *.cof *.hxl *.lst *.obj *.rlf *.sdb
	rm -f funclist
