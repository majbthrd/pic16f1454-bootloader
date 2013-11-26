# customize the following paths for your computer
CC = "C:\Program Files\Microchip\xc8\v1.20\bin\xc8.exe"
LIB_INC_PATH = "C:/Microchip/Include"

# customize for your part; likely choices are: 16F1454, 16F1455, or 16F1459
CHIP = 16F1454

#uncomment the following if you wish to exclude the HIDHYBRID mode from the build
#OMIT_HIDHYBRID = y

CFLAGS = --chip=$(CHIP) -Q -G  --double=24 --float=24
CFLAGS += --rom=default,-1000-1FFF 
CFLAGS += --opt=default,+asm,-asmfile,+speed,-space,-debug --addrqual=ignore
CFLAGS += --mode=pro -N64 -I. -I$(LIB_INC_PATH) --warn=0 --asmlist --summary=default,-psect,-class,+mem,-hex,-file --output=default,-inhx032 
CFLAGS += --runtime=default,+clear,+init,-keep,-no_startup,+osccal,-resetbits,-download,-stackcall,+clib
ifdef OMIT_HIDHYBRID
CFLAGS += -DOMIT_HIDHYBRID
else
CFLAGS += --ram=default,-70-7b,-f0-fb,-170-17b,-1f0-1fb,-220-26f,-270-27b,-2a0-2ef,-2f0-2fb,-320-36f,-370-37b,-3a0-3ef,-3f0-3fb,-420-46f,-470-47b,-4a0-4ef,-4f0-4fb,-520-56f,-570-57b,-5a0-5ef,-5f0-5fb,-620-66f,-670-67b,-6a0-6ef,-6f0-6fb,-720-76f,-770-77b,-7a0-7ef,-7f0-7fb,-820-86f,-870-87b,-8a0-8ef,-8f0-8fb,-920-96f,-970-97b,-9a0-9ef,-9f0-9fb,-a20-a6f,-a70-a7b,-aa0-aef,-af0-afb,-b20-b6f,-b70-b7b,-ba0-bef,-bf0-bfb,-c20-c6f,-c70-c7b,-ca0-cef,-cf0-cfb,-d20-d6f,-d70-d7b,-da0-def,-df0-dfb,-e20-e6f,-e70-e7b,-ea0-eef,-ef0-efb,-f20-f6f,-f70-f7b,-fa0-fef,-ff0-ffb 
endif

HIDPLUS_OBJS = usb_device.p1 usb_function_hid.p1 hidplus.p1 hidplus_descriptors.p1
HIDPLUS_HDRS = usb_config.h HardwareProfile.h

all: hidplus.hex

hidplus.hex: $(HIDPLUS_OBJS)
	$(CC) $(CFLAGS) -o./$@ $(HIDPLUS_OBJS)

%.p1: %.c $(HIDPLUS_HDRS) Makefile
	$(CC) --pass1 $(CFLAGS) -o./$@ $<

clean:
	rm -f hidplus.hex
	rm -f *.p1 *.d *.pre *.sym *.cmf *.cof *.hxl *.lst *.obj *.rlf *.sdb
	rm -f funclist
