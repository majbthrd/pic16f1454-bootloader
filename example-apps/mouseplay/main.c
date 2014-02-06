/*
    example HID mouse using PIC16F1454 microcontroller

    Copyright (C) 2014 Peter Lawrence

    Permission is hereby granted, free of charge, to any person obtaining a 
    copy of this software and associated documentation files (the "Software"), 
    to deal in the Software without restriction, including without limitation 
    the rights to use, copy, modify, merge, publish, distribute, sublicense, 
    and/or sell copies of the Software, and to permit persons to whom the 
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in 
    all copies or substantial portions of the Software.

    The software is for use solely and exclusively on Microchip PIC 
    Microcontroller products. 

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
    DEALINGS IN THE SOFTWARE.
*/

#include "./USB/usb.h"
#include "./USB/usb_function_hid.h"

/* 
since this is a downloaded app, configuration words (e.g. __CONFIG or #pragma config) are not relevant
*/

/* USB data buffer (HID mouse has only "IN" (device to PC) report) */
unsigned char hid_report_in[HID_INT_IN_EP_SIZE] @0xA0;

/* counter incremented by USB SOF (Start Of Frame) to track approximate time between reports */
static BYTE tick_count;

/* arbitrary mouse movement pattern to play back */
ROM signed char move_table[]=
{
	/* 
	X, Y, (at time 0)
	X, Y, (at time 1)
	X, Y, (at time 2)
	...
	*/
	6, -2,
	2, -6,
	-2, -6,
	-6, -2,
	-6, 2,
	-2, 6,
	2, 6,
	6, 2,
};

/* handles used for HID functionality */
USB_VOLATILE USB_HANDLE USBInHandle;

void interrupt ISRCode(void)
{
}

int main(void)
{
	BYTE table_index = 0;

	/* initialize the USB driver */
	USBInHandle = 0;
	USBDeviceInit();
	USBDeviceAttach();

	for (;;)
	{
		USBDeviceTasks(); // polling method of USB library

		// User Application USB tasks
		if( (USBDeviceState < CONFIGURED_STATE) || (1 == USBSuspendControl) )
			continue;

		if (!HIDTxHandleBusy(USBInHandle))
		{
			if (tick_count > 63) /* if more than 64 ms has passed since the last report... */
			{
				/* table_index modulus 16 *AND* make table_index an even number */
				table_index &= 0xE;

				/* build HID report */
				hid_report_in[0] = 0;
				hid_report_in[1] = move_table[table_index++];
				hid_report_in[2] = move_table[table_index++];

				/* transmit HID report */
				USBInHandle = HIDTxPacket(HID_EP, (BYTE*)hid_report_in, 3);

				tick_count = 0;
			}
		}
	}
}

BOOL USER_USB_CALLBACK_EVENT_HANDLER(int event, void *pdata, WORD size)
{
	/*
	if-statement, rather than switch(), is used here as Microchip's compiler can be very inefficient with switch()
	*/
	if (EVENT_SOF == event)
	{
        	tick_count++;
	}
	else if (EVENT_CONFIGURED == event)
	{
        	// enable the HID endpoint
		USBEnableEndpoint(HID_EP, USB_IN_ENABLED | USB_HANDSHAKE_ENABLED | USB_DISALLOW_SETUP);
	}
	else if (EVENT_EP0_REQUEST == event)
	{
		USBCheckHIDRequest();
	}

	return TRUE; 
}
