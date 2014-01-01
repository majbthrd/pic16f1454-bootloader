/*
    multi-platform GUI Download tool for
    HID bootloader for PIC16F1454/PIC16F1455/PIC16F1459 microcontroller

    Copyright (C) 2013 Peter Lawrence

    Permission is hereby granted, free of charge, to any person obtaining a 
    copy of this software and associated documentation files (the "Software"), 
    to deal in the Software without restriction, including without limitation 
    the rights to use, copy, modify, merge, publish, distribute, sublicense, 
    and/or sell copies of the Software, and to permit persons to whom the 
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in 
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
    DEALINGS IN THE SOFTWARE.
*/

//#include <stdint.h>
#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Progress.H>
#include "hidapi.h"

#define MARGIN_SIZE    12
#define BUTTON_HEIGHT  32
#define BUTTON_WIDTH   96
#define PROGRESS_WIDTH 128
#define WINDOW_HEIGHT  (2 * BUTTON_HEIGHT + 3 * MARGIN_SIZE)
#define WINDOW_WIDTH  ((BUTTON_WIDTH + PROGRESS_WIDTH) + 3 * MARGIN_SIZE)

#define HID_BUFFER_SIZE 65

static void hex_button_cb(Fl_Widget *p, void *data);
static void flash_button_cb(Fl_Widget *p, void *data);

static unsigned readhex(const char *text, unsigned digits);
static int xfer(hid_device *handle, unsigned char *data, int txlen);

static Fl_Window *win;
static Fl_Progress *progress;
static Fl_Button *hex_button, *flash_button;
static Fl_File_Chooser *fc;

static unsigned char image[8192];

int main (int argc, char *argv[])
{
	win = new Fl_Window(WINDOW_WIDTH, WINDOW_HEIGHT, "Firmware Update");

	win->begin();

	hex_button = new Fl_Button(PROGRESS_WIDTH + 2 * MARGIN_SIZE, MARGIN_SIZE, BUTTON_WIDTH, BUTTON_HEIGHT, "Load HEX");
	hex_button->callback(hex_button_cb, NULL);

	flash_button = new Fl_Button(PROGRESS_WIDTH + 2 * MARGIN_SIZE, BUTTON_HEIGHT + 2 * MARGIN_SIZE, BUTTON_WIDTH, BUTTON_HEIGHT, "Program");
	flash_button->callback(flash_button_cb, NULL);
	flash_button->deactivate();

	fc = new Fl_File_Chooser(".", "Intel Hex files (*.{hex})", Fl_File_Chooser::SINGLE, "pick PIC16F1454 firmware file");

	win->end();

	win->show();

	Fl::run();

	return 0;
}

static void hex_button_cb(Fl_Widget *p, void *data)
{
	FILE *input;
	char line[256];
	unsigned count, address;
	const char *ptr;
	struct
	{
		unsigned out_of_bounds:1;
		unsigned extended:1;
	} flags;
	unsigned short word, crc;

	flash_button->deactivate();

	memset(&flags, 0, sizeof(flags));

	memset(image, 0xFF, sizeof(image));

	fc->show();

	while(fc->shown())
	      Fl::wait();

	input = fopen(fc->value(), "ra");

	if (NULL == input)
	{
		return;
	}

	while (!feof(input))
	{
		if (fgets(line, sizeof(line), input))
		{
			if (':' == line[0])
			{
				count = readhex(line + 1, 2);
				address = readhex(line + 3, 4);
				if (0 == strncmp(line + 7, "00", 2) && !flags.extended) /* data record */
				{
					ptr = line + 9;
					while (count--)
					{
						if ( (address >= 0x2000) && (address < 0x3FFE) )
						{
							image[address - 0x2000] = readhex(ptr, 2);
						}
						else
						{
printf("%x\n", address);
							flags.out_of_bounds = 1;
						}
						address++;
						ptr += 2;
					}
				}
				else if (0 == strncmp(line + 7, "04", 2)) /* subsequent data is not program code */
				{
					flags.extended = 1;
				}
				else if (0 == strncmp(line + 7, "01", 2)) /* end of file record */
				{
					break;
				}
			}
		}
	}

	fclose(input);

	if (flags.out_of_bounds)
		if (2 != fl_choice("File contains data outside the bounds of the user-programmable area\nDo you wish to proceed despite this?", NULL, "No", "Yes"))
			return;

	/*
	compute USERCODE mode CRC
	*/

	crc = 0;

	for (address = 0; address < 0x1FFE; address+=2)
	{
		word = image[address + 1];
		word <<= 8;
		word += image[address + 0];

		/* update CRC over the 14 bits of program memory data */
		for (count = 0; count < 14; count++)
		{
		    if ((crc & 0x0001) ^ (word & 0x0001))
		        crc = (crc >> 1) ^ 0x23B1;
		    else
		        crc >>= 1;
			word >>= 1;
		}
	}

#if 0
	crc ^= 0x3FFF; /* for HIDHYBRID code images */
#endif

	image[0x1FFF] = (unsigned char)((crc & 0xFF00) >> 8);
	image[0x1FFE] = (unsigned char)((crc & 0x00FF) >> 0);

	flash_button->activate();
}

static unsigned readhex(const char *text, unsigned digits)
{
	unsigned result = 0;

	while (digits--)
	{
		result <<= 4;
		
		if ( (*text >= '0') && (*text <= '9') )
			result += *text - '0';
		else if ( (*text >= 'A') && (*text <= 'F') )
			result += 10 + *text - 'A';
		else if ( (*text >= 'a') && (*text <= 'f') )
			result += 10 + *text - 'a';

		text++;
	}

	return result;
}

static void flash_button_cb(Fl_Widget *p, void *data)
{
	unsigned char buf[HID_BUFFER_SIZE];
	hid_device *handle;
	unsigned index, address, count;
	int res = 0;

	if (hid_init() != 0)
	{
		fl_alert("unable to open HIDAPI");
		return;
	}

	handle = hid_open(0x04D8, 0x003F, NULL);

	if (!handle)
	{
		fl_alert("a USB device with the bootloader was not to be found");
		return;
	}

	memset(buf, 0, sizeof(buf));

	for (address = 0; address < 0x2000; )
	{
		index = (address + 0x2000) >> 1;

		buf[0] = 0x00;
		buf[1] = 0x81; /* erase memory */
		buf[2] = (unsigned char)((index & 0xFF00) >> 8);
		buf[3] = (unsigned char)((index & 0x00FF) >> 0);

		res = xfer(handle, buf, HID_BUFFER_SIZE);

		if (-1 == res)
			break;

		buf[0] = 0x00;
		buf[1] = 0x82; /* program memory */
		buf[2] = (unsigned char)((index & 0xFF00) >> 8);
		buf[3] = (unsigned char)((index & 0x00FF) >> 0);

		for (count = 0; count < 32; count+=2)
		{
			buf[4 + count + 0] = image[address + 1];
			buf[4 + count + 1] = image[address + 0];
			address += 2;
		}

		res = xfer(handle, buf, HID_BUFFER_SIZE);

		if (-1 == res)
			break;

		index += 16;

		buf[0] = 0x00;
		buf[1] = 0x82; /* program memory */
		buf[2] = (unsigned char)((index & 0xFF00) >> 8);
		buf[3] = (unsigned char)((index & 0x00FF) >> 0);

		for (count = 0; count < 32; count+=2)
		{
			buf[4 + count + 0] = image[address + 1];
			buf[4 + count + 1] = image[address + 0];
			address += 2;
		}

		res = xfer(handle, buf, HID_BUFFER_SIZE);

		if (-1 == res)
			break;
	}

	if (-1 == res)
		fl_alert("programming was NOT successful");
	else
	{
		fl_alert("programming was successful!\ndisconnect and reconnect device to boot new code");
	}

	hid_close(handle);
	hid_exit();
}

/* Send a message and receive the reply */
static int xfer(hid_device *handle, unsigned char *data, int txlen)
{
	/* write data to device */
	int retval = hid_write(handle, data, txlen);

	if (retval == -1)
	{
		return -1;
	}

	/* get reply */
//	retval = hid_read_timeout(handle, data, HID_BUFFER_SIZE, 1000);
	retval = hid_read(handle, data, HID_BUFFER_SIZE);
	if (retval == -1 || retval == 0)
	{
		return -1;
	}

	return 0;
}

