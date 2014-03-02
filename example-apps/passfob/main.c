/*
    example HID keyboard using PIC16F1454 microcontroller

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

/*
this determines how long one has to press the button for the transmit to be triggered
it also determines how long the button must be released in order to re-arm
*/
#define MIN_TRIGGER_MS 750

/*
this determines which bit to monitor for the bootloader entry
*/
#define BOOTLOADER_ENTRY_KEYLOCK_MASK 0x04 /* scroll-lock key */

/* USB data buffers (HID keyboard has both "IN" (device to PC) and "OUT" (PC to device) reports) */
unsigned char hid_report_in[HID_INT_IN_EP_SIZE] @0xA0;
volatile unsigned char hid_report_out[HID_INT_OUT_EP_SIZE] @0x120;

/* flag set USB SOF (Start Of Frame) to track approximate time */
static BOOL ms_tick = FALSE;

/* arbitrary key strokes to play back */
ROM char key_table[] =
{
	0x4, // 'a'
	0x5, // 'b'
	0x6, // 'c'
	0x7, // 'd'
};

/* handles used for HID functionality */
USB_VOLATILE USB_HANDLE USBInHandle;
USB_VOLATILE USB_HANDLE USBOutHandle;

void interrupt ISRCode(void)
{
}

int main(void)
{
	BYTE table_index = 0;
	unsigned short count = MIN_TRIGGER_MS;
	enum
	{
		COOLDOWN,
		ARMED,
		TRANSMITTING,
	} state = COOLDOWN;
	unsigned short keylock_tick_count = 0;
	BYTE last_keylock_state = 0;

	/* enable pull-up on RA3 (for pushbutton detection) */
	OPTION_REGbits.nWPUEN = FALSE;
	WPUA = (1 << 3);

	/* initialize the USB driver */
	USBInHandle = 0;
	USBOutHandle = 0;
	USBDeviceInit();
	USBDeviceAttach();

	for (;;)
	{
		USBDeviceTasks(); // polling method of USB library

		// User Application USB tasks
		if( (USBDeviceState < CONFIGURED_STATE) || (1 == USBSuspendControl) )
			continue;

		if (ms_tick)
		{
			if ( (COOLDOWN == state) || (ARMED == state) )
			{
				if (PORTAbits.RA3)
				{
					if (count)
						count--;
					else
						state = ARMED;
				}
				else
				{
					if (count < MIN_TRIGGER_MS)
						count++;
					else if (ARMED == state)
					{
						state = TRANSMITTING;
						table_index = 0;
					}
				}
			}

			/* if chosen KEYLOCK is now on, increment keylock_tick_count until it reaches 65535 */
			if (last_keylock_state & BOOTLOADER_ENTRY_KEYLOCK_MASK)
				if (0xFFFF != keylock_tick_count)
					keylock_tick_count++;

			ms_tick = FALSE;
		}

		if (!HIDTxHandleBusy(USBInHandle))
		{
			/* build HID report */
			hid_report_in[0] = 0;
			hid_report_in[1] = 0;
			if (TRANSMITTING == state)
			{
				if (0 == (table_index & 1))
					hid_report_in[2] = key_table[table_index >> 1];
				else
					hid_report_in[2] = 0;

				table_index++;

				if ((2 * sizeof(key_table)) == table_index)
					state = COOLDOWN;
			}
			hid_report_in[3] = 0;
			hid_report_in[4] = 0;
			hid_report_in[5] = 0;
			hid_report_in[6] = 0;
			hid_report_in[7] = 0;

			/* transmit HID report */
			USBInHandle = HIDTxPacket(HID_EP, (BYTE*)hid_report_in, sizeof(hid_report_in));
		}

	        if (!HIDRxHandleBusy(USBOutHandle))
        	{
			/*
			we monitor KEYLOCK activity to provide a means to force the device back to the bootloader
			reboot to bootloader *if* KEYLOCK turns on for a second (800 to 1200 milliseconds) and then goes back off
			*/

			if (hid_report_out[0] != last_keylock_state)
			{
				if ( (keylock_tick_count > 800) && (keylock_tick_count < 1200) )
				{
					/* enable watchdog; the code doesn't clear the watchdog, so it will eventually reset */
					WDTCONbits.SWDTEN = 1;
				}

				keylock_tick_count = 0;

				last_keylock_state = hid_report_out[0];
			}

			/* Re-arm the OUT endpoint */
			USBOutHandle = HIDRxPacket(HID_EP, (BYTE*)&hid_report_out, sizeof(hid_report_out));
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
        	ms_tick = TRUE;
	}
	else if (EVENT_CONFIGURED == event)
	{
        	// enable the HID endpoint
		USBEnableEndpoint(HID_EP, USB_IN_ENABLED | USB_OUT_ENABLED | USB_HANDSHAKE_ENABLED | USB_DISALLOW_SETUP);
		USBOutHandle = HIDRxPacket(HID_EP, (BYTE*)&hid_report_out, sizeof(hid_report_out));
	}
	else if (EVENT_EP0_REQUEST == event)
	{
		USBCheckHIDRequest();
	}

	return TRUE; 
}
