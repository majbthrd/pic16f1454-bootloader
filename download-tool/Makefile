DOWNLOAD_CFLAGS  = -fstack-protector -fstack-protector-all
DOWNLOAD_CFLAGS += $(shell fltk-config --cxxflags --ldstaticflags)
DOWNLOAD_CFLAGS +=-ludev `pkg-config libusb-1.0 --libs`

HIDAPI_CFLAGS += `pkg-config libusb-1.0 --cflags`
HIDAPI_CFLAGS += `pkg-config libusb-1.0 libudev --libs`

all: download

hid-libusb.o: ./linux/hid-libusb.c
	gcc -c ./linux/hid-libusb.c -o $@ $(HIDAPI_CFLAGS) -L.

download: Makefile download.cpp hid-libusb.o
	g++ download.cpp hid-libusb.o -Os -o $@ $(DOWNLOAD_CFLAGS) -L.
	strip $@

clean:
	rm -f download
	rm -f hid-libusb.o

