DOWNLOAD_CFLAGS  = -fstack-protector -fstack-protector-all
DOWNLOAD_CFLAGS += $(shell fltk-config --cxxflags --ldflags)
DOWNLOAD_CFLAGS +=-ludev `pkg-config libusb-1.0 --libs`

HIDAPI_CFLAGS += `pkg-config libusb-1.0 --cflags`
HIDAPI_CFLAGS += `pkg-config libusb-1.0 libudev --libs`

all: download

hid-libusb.o: hid.c
	gcc -c hid.c -o $@ $(HIDAPI_CFLAGS) -L.

download: Makefile download.cpp hid-libusb.o
	g++ download.cpp hid-libusb.o -Os -o $@ $(DOWNLOAD_CFLAGS) -L.
	strip $@

clean:
	rm -f download

