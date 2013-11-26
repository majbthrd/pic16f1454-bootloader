CFLAGS = -fstack-protector -fstack-protector-all
CFLAGS += $(shell fltk-config --cxxflags --ldflags)

LIBS=-ludev `pkg-config libusb-1.0 --libs`

DOWNLOAD_C = download.cpp hid-libusb.o

all: download

download: Makefile $(DOWNLOAD_C)
	g++ $(DOWNLOAD_C) $(LIBS) -Os -o $@ $(CFLAGS) -L.
	strip $@

clean:
	rm -f download

