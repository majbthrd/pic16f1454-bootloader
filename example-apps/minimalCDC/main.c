/*
    example minimal CDC serial port adapter using PIC16F1454 microcontroller

    this is specific to the PIC16F1454; TX is on pin RC4 and RX on pin RC5

    based on M-Stack by Alan Ott, Signal 11 Software

    culled from USB CDC-ACM Demo (by Alan Ott, Signal 11 Software)
    and ANSI C12.18 optical interface (by Peter Lawrence)

    Copyright (C) 2014,2015 Peter Lawrence

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

#include "usb.h"
#include <xc.h>
#include <string.h>
#include "usb_config.h"
#include "usb_ch9.h"
#include "usb_cdc.h"

/* local data buffers for CDC functionality */
static uint8_t PC2PIC_Buffer[EP_2_LEN];
static uint8_t PIC2PC_Buffer[EP_2_LEN];

/* variables to track positions and occupancy in local CDC buffers */
uint8_t PIC2PC_pending_count;
size_t  PC2PIC_buffer_occupancy;
uint8_t PC2PIC_read_index;

static void InitializeUSART(void);

int main(void)
{
	uint8_t pc2pic_data_ready;
	uint8_t *in_buf;
	const uint8_t *out_buf;
	int i;

	pc2pic_data_ready = 0;

	InitializeUSART();

	PIC2PC_pending_count = 0;
	PC2PIC_buffer_occupancy = 0;

/* Configure interrupts, per architecture */
#ifdef USB_USE_INTERRUPTS
	INTCONbits.PEIE = 1;
	INTCONbits.GIE = 1;
#endif

	usb_init();

	for (;;)
	{
#ifndef USB_USE_INTERRUPTS
		usb_service();
#endif

		/* if USB isn't configured, there is no point in proceeding further */
		if (!usb_is_configured())
			continue;

		/* if the USART has received another byte, add it to the queue if there is room */
		if (PIR1bits.RCIF && (PIC2PC_pending_count < (EP_2_LEN - 1)))
		{
			if (RCSTAbits.OERR)
				RCSTAbits.CREN = 0;  /* in case of overrun error, reset the port */
			PIC2PC_Buffer[PIC2PC_pending_count] = RCREG;
			RCSTAbits.CREN = 1;  /* and then (re-)enable receive */
			++PIC2PC_pending_count;
		}

		/* if our PC2PIC buffer is not empty *AND* the USART can accept another byte, transmit another byte */
		if (pc2pic_data_ready && TXSTAbits.TRMT)
		{
			/* transmit another byte from buffer */
			TXREG = PC2PIC_Buffer[PC2PIC_read_index];
			/* update the read index */
	    		++PC2PIC_read_index;
			/* if the read_index has reached the occupancy value, signal that the buffer is empty again */
	    		if (PC2PIC_read_index == PC2PIC_buffer_occupancy)
	    			pc2pic_data_ready = 0;
		}

		/* proceed further only if the PC can accept more data */
		if (usb_in_endpoint_halted(2) || usb_in_endpoint_busy(2))
			continue;

		/* if we've reached here, the USB stack can accept more; if we have PIC2PC data to send, we hand it over */
		if (PIC2PC_pending_count > 0)
		{
			in_buf = usb_get_in_buffer(2);
			memcpy(in_buf, PIC2PC_Buffer, PIC2PC_pending_count);
			usb_send_in_buffer(2, PIC2PC_pending_count);
			PIC2PC_pending_count = 0;
		}

		/* if our PC2PIC buffer is NOT empty, we won't go further (where we would try to accept more from the PC) */
		if (pc2pic_data_ready)
			continue;

		/* if we pass this test, we are committed to make the usb_arm_out_endpoint() call */
		if (!usb_out_endpoint_has_data(2))
			continue;

		/* ask USB stack for more PC2PIC data */
		PC2PIC_buffer_occupancy = usb_get_out_buffer(2, &out_buf);

		/* if there was any, put it in the buffer and update the state variables */
		if(PC2PIC_buffer_occupancy > 0)
		{
			memcpy(PC2PIC_Buffer, out_buf, PC2PIC_buffer_occupancy);
			/* signal that the buffer is not empty */
			pc2pic_data_ready = 1;
			/* rewind read index to the beginning of the buffer */
			PC2PIC_read_index = 0;
		}

		usb_arm_out_endpoint(2);
	}
}

/* Callbacks. These function names are set in usb_config.h. */
void app_set_configuration_callback(uint8_t configuration)
{

}

uint16_t app_get_device_status_callback()
{
	return 0x0000;
}

void app_endpoint_halt_callback(uint8_t endpoint, bool halted)
{

}

int8_t app_set_interface_callback(uint8_t interface, uint8_t alt_setting)
{
	return 0;
}

int8_t app_get_interface_callback(uint8_t interface)
{
	return 0;
}

void app_out_transaction_callback(uint8_t endpoint)
{

}

void app_in_transaction_complete_callback(uint8_t endpoint)
{

}

int8_t app_unknown_setup_request_callback(const struct setup_packet *setup)
{
	/* To use the CDC device class, have a handler for unknown setup
	 * requests and call process_cdc_setup_request() (as shown here),
	 * which will check if the setup request is CDC-related, and will
	 * call the CDC application callbacks defined in usb_cdc.h. For
	 * composite devices containing other device classes, make sure
	 * MULTI_CLASS_DEVICE is defined in usb_config.h and call all
	 * appropriate device class setup request functions here.
	 */
	return process_cdc_setup_request(setup);
}

int16_t app_unknown_get_descriptor_callback(const struct setup_packet *pkt, const void **descriptor)
{
	return -1;
}

void app_start_of_frame_callback(void)
{

}

void app_usb_reset_callback(void)
{

}

/* CDC Callbacks. See usb_cdc.h for documentation. */

int8_t app_send_encapsulated_command(uint8_t interface, uint16_t length)
{
	return -1;
}

int16_t app_get_encapsulated_response(uint8_t interface,
                                      uint16_t length, const void **report,
                                      usb_ep0_data_stage_callback *callback,
                                      void **context)
{
	return -1;
}

void app_set_comm_feature_callback(uint8_t interface,
                                     bool idle_setting,
                                     bool data_multiplexed_state)
{

}

void app_clear_comm_feature_callback(uint8_t interface,
                                       bool idle_setting,
                                       bool data_multiplexed_state)
{

}

int8_t app_get_comm_feature_callback(uint8_t interface,
                                     bool *idle_setting,
                                     bool *data_multiplexed_state)
{
	return -1;
}

void app_set_line_coding_callback(uint8_t interface,
                                    const struct cdc_line_coding *coding)
{

}

int8_t app_get_line_coding_callback(uint8_t interface,
                                    struct cdc_line_coding *coding)
{
	/* This is where baud rate, data, stop, and parity bits are set. */
	return -1;
}

int8_t app_set_control_line_state_callback(uint8_t interface,
                                           bool dtr, bool dts)
{
	return 0;
}

int8_t app_send_break_callback(uint8_t interface, uint16_t duration)
{
	return 0;
}

void interrupt isr()
{
#ifdef USB_USE_INTERRUPTS
    usb_service();
#endif
}

static void InitializeUSART(void)
{
	/* RX on RC5 is an input */
        TRISCbits.TRISC5=1;

	/* TX on RC4 is an output */
        TRISCbits.TRISC4=0;

	TXSTA = 0x24;
	RCSTA = 0x90;

	/* 115200 bps */
	SPBRG  = 103;
	SPBRGH = 0;

	BAUDCON = 0x08;

	/* clear any data in receive buffer */
	(volatile void)RCREG;
}
