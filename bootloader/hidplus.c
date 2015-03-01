/*
    USB HID bootloader plus CDC for PIC16F1454/PIC16F1455/PIC16F1459 microcontroller

    Copyright (C) 2013,2014 Peter Lawrence

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
#ifdef ADD_CDC
#include "./USB/usb_function_cdc.h"
#endif

/* state of configuration words */
__CONFIG(FOSC_INTOSC & WDTE_SWDTEN & PWRTE_ON & MCLRE_OFF & CP_ON & BOREN_ON & CLKOUTEN_OFF & IESO_OFF & FCMEN_OFF);
__CONFIG(WRT_HALF & CPUDIV_NOCLKDIV & USBLSCLK_48MHz & PLLMULT_3x & PLLEN_ENABLED & STVREN_ON & BORV_LO & LPBOR_OFF & LVP_OFF);

/* UART bps of 115200 */
#pragma config IDLOC0 = 103
#pragma config IDLOC1 = 0

/* local data buffers for HID functionality */
unsigned char RxDataBuffer[HID_INT_EP_SIZE] @0xA0;
unsigned char TxDataBuffer[HID_INT_EP_SIZE] @0x120;

#ifdef ADD_CDC
/* CDC variables used by CDC stack... declared here so that they are visible, rather than latent in the stack code */
volatile FAR CDC_NOTICE cdc_notice; 
volatile FAR unsigned char cdc_data_rx[CDC_DATA_OUT_EP_SIZE]; 
volatile FAR unsigned char cdc_data_tx[CDC_DATA_IN_EP_SIZE]; 
LINE_CODING line_coding;

/* local data buffers for CDC functionality */
char PC2PIC_Buffer[CDC_DATA_OUT_EP_SIZE];
char PIC2PC_Buffer[CDC_DATA_IN_EP_SIZE];
#endif

/* handles used for HID functionality */
USB_HANDLE USBHIDOutHandle;
USB_HANDLE USBHIDInHandle;

/* prototyping of external USB descriptors in ROM */
extern ROM USB_DEVICE_DESCRIPTOR bootloader_device_dsc;
extern ROM BYTE *ROM CDC_USB_CD_Ptr[];
extern ROM BYTE *ROM CDC_USB_SD_Ptr[];

/* pointers that pick one of the above external USB descriptors */
USB_DEVICE_DESCRIPTOR const *pnt_device_dsc;
const BYTE const **USB_CD_Ptr;
const BYTE const **USB_SD_Ptr;

#ifdef ADD_CDC
/* variables to track positions and occupancy in local CDC buffers */
unsigned char PIC2PC_pending_count;
unsigned char PC2PIC_buffer_occupancy;
unsigned char PC2PIC_read_index;

/* prototyping of USART functions */
void InitializeUSART(void);
#endif

/*
hidden away in Section 5.10.1.4 of the XC8 manual is disclosure that, by default, the STATUS bits are clobbered
by using the --runtime=+resetbits argument to the XC8 compiler, the following additional symbols are defined
*/
extern unsigned char __resetbits;
extern bit __powerdown;
extern bit __timeout;

void interrupt ISRCode(void)
{
#asm
LJMP 0x1004 /* call user code interrupt vector */
#endasm
}

/* bit field definitions for "flags" variable in main() */
#define FLAG_USERCODE        0x01
#define FLAG_PC2PIC_DATA_RDY 0x02
#define FLAG_PASSED_CRC      0x04

int main(void)
{
    WORD crc, data;
    BYTE index, lo, hi;
    BYTE tx_count;
    BYTE flags;
    
    /* enable pull-up on RA3 (for boot mode detection) */
    OPTION_REGbits.nWPUEN = FALSE;
    WPUA = (1 << 3);

    /* magic numbers from Microchip */
    OSCTUNE = 0;
    OSCCON = 0xFC; /* 16MHz HFINTOSC with 3x PLL enabled (48MHz operation) */
    ACTCON = 0x90; /* Enable active clock tuning with USB */

    /*
    pull-up on RA3 needs a moment to do its thing
    so, now is a good time to spend time computing a CRC of the user-programmable area
    */

    flags = 0;

    /* initialize CRC */
    crc = 0;

    /* use local copy of PMADRH:PMADRL to prevent XC8 compilation issues */
    lo = 0x00;
    hi = 0x10;

    for (;;)
    {
        PMADRH = hi;
        PMADRL = lo;

        /* clear CFGS (we're reading Program, not Configuration, Memory) */
        PMCON1bits.CFGS = FALSE;
        /* set RD (initiate read) */
        PMCON1bits.RD = TRUE;
        /* mandatory two nops */
        _nop(); _nop();

        /* retrieve result from program memory and put it into a WORD */
        data = PMDATH;
        data <<= 8;
        data += PMDATL;

        /* increment program memory address, and bail if we've just read the last word */
        lo++;
        if (0 == lo)
        {
            hi++;
            if (data == crc)
            {
                /* we've reached a 256 word boundary *and* the CRC passes, so we note this */
                /* we choose NOT to exit the loop here in order to have a consistent amount of time for the pull-up on RA3 */
                flags |= FLAG_PASSED_CRC;
            }
            if (0x20 == hi)
            {
                /* we've reached the last word */
                break;
            }
        }
        
        /* update CRC over the 14 bits of program memory data */
        for (index = 0; index < 14; index++)
        {
            if ((crc & 0x0001) ^ (data & 0x0001))
                crc = (crc >> 1) ^ 0x23B1;
            else
                crc >>= 1;
            data >>= 1;
        }
    }

    /*
    whilst we've been busy, pull-up on RA3 has now had a chance to work
    make determination as to whether to not use bootloader
    */
    if (PORTAbits.RA3)
    {
        /* the XC8 symbol "__timeout" means *NOT timeout*... makes sense, right!? :( */
        if ( (flags & FLAG_PASSED_CRC) && __timeout )
            flags |= FLAG_USERCODE;
    }

    /* disable pull-up now that determination has been made */
    OPTION_REGbits.nWPUEN = TRUE;
    WPUA = 0;

    /* if not in bootloader mode, branch to user code (for initialization or outright different USB code) */
    if (flags & FLAG_USERCODE)
    {
        /*
        in USERIMAGE boot mode, we goto ("LONG jump") the user's code with no expectation of returning
        doing a call would also function, but would waste one entry on the stack
        */
        asm("ljmp 0x1000"); /* call user code reset vector */
    }

    /* pick which descriptors to use; in this application, there is another one choice */
    pnt_device_dsc = &bootloader_device_dsc;
    USB_CD_Ptr = (const BYTE const **)CDC_USB_CD_Ptr;
    USB_SD_Ptr = (const BYTE const **)CDC_USB_SD_Ptr;

#ifdef ADD_CDC
    InitializeUSART();
#endif

    /*
    initialize the USB driver
    */

#ifdef ADD_CDC
    PIC2PC_pending_count = 0;
    PC2PIC_buffer_occupancy = 0;
#endif

    USBHIDOutHandle = 0;
    USBHIDInHandle = 0;

    USBDeviceInit();
    USBDeviceAttach();

    /*
    commence main loop
    */

    for (;;)
    {
        USBDeviceTasks(); // polling method of USB library

        // User Application USB tasks
        if( (USBDeviceState < CONFIGURED_STATE) || (1 == USBSuspendControl) )
            continue;

        //Check if we have received an OUT data packet from the host
        if (!HIDRxHandleBusy(USBHIDOutHandle))
        {
            /*
            if tx_count is set to be non-zero by subsequent code, this indicates data
            of length tx_count is in DataBuffer[] to be passed to the USB driver
            */
            tx_count = 0;

            if (!HIDTxHandleBusy(USBHIDInHandle))
            {
                /*
                parse incoming HID packet and generate response
                */
                {
                    TxDataBuffer[tx_count++] = RxDataBuffer[0];
                    TxDataBuffer[tx_count++] = RxDataBuffer[1];
                    TxDataBuffer[tx_count++] = RxDataBuffer[2];

                    hi = RxDataBuffer[1];
                    lo = RxDataBuffer[2];

                    /* clear CFGS for accessing Program Memory; set accessing Configuration Memory */
       	            PMCON1bits.CFGS = (RxDataBuffer[0] & 0x04) ? TRUE : FALSE;

                    switch (RxDataBuffer[0])
                    {
                    case 0x80:  /* Read Memory */
                    case 0x84:  /* Read Config */
                        do
                        {
                            /* set starting Program Memory address */
                            PMADRH = hi;
                            PMADRL = lo;
                            /* set RD (initiate read) */
                            PMCON1bits.RD = TRUE;
                            /* mandatory two nops */
                            _nop(); _nop();
                            /* retrieve result */
                            TxDataBuffer[tx_count++] = PMDATH;
                            TxDataBuffer[tx_count++] = PMDATL;
                            /* increment program memory address */
                            lo++;
                            if (0 == lo)
                                hi++;
                        } while (tx_count < (32 + 3));
                        break;

                    case 0x81:  /* Erase Memory */
                    case 0x85:  /* Erase Config */
                        /* provide Program Memory row address */
                        PMADRH = hi;
                        PMADRL = lo;
                        /* select erase operation */
                        PMCON1bits.FREE = TRUE;
                        /* enable write/erase operation */
                        PMCON1bits.WREN = TRUE;
                        /* unlock sequence */
                        PMCON2 = 0x55;
                        PMCON2 = 0xAA;
                        PMCON1bits.WR = TRUE;
                        /* mandatory two nops */
                        _nop(); _nop();
                        /* disable write/erase operation */
                        PMCON1bits.WREN = FALSE;
                        break;

                    case 0x82:  /* Program Memory */
                    case 0x86:  /* Program Config */
                        /* provide Program Memory row address */
                        PMADRH = hi;
                        PMADRL = lo;
                        /* select write operation */
                        PMCON1bits.FREE = FALSE;
                        /* load write latches only */
                        PMCON1bits.LWLO = TRUE;
                        /* enable write/erase operation */
                        PMCON1bits.WREN = TRUE;

                        index = 3;

                        for (;;)
                        {
                            PMDATH = RxDataBuffer[index++];
                            PMDATL = RxDataBuffer[index++];
                            if ( (index >= (32 + 3)) || PMCON1bits.CFGS )
                            {
                                /* write latches to flash */
                                PMCON1bits.LWLO = FALSE;
                            }
                            /* unlock sequence */
                            PMCON2 = 0x55;
                            PMCON2 = 0xAA;
                            PMCON1bits.WR = TRUE;
                            /* mandatory two nops */
                            _nop(); _nop();
                            if ( (index >= (32 + 3)) || PMCON1bits.CFGS )
                            {
                                /* we've finished, so bail */
                                break;
                            }
                            /* increment program memory address */
                            lo++;
                            if (0 == lo)
                                hi++;
                            PMADRL = lo;
                        }
                        /* disable write/erase operation */
                        PMCON1bits.WREN = FALSE;
                        break;
                    }
                }
            }


            if (tx_count)
                USBHIDInHandle = HIDTxPacket(HID_EP, (BYTE*)&TxDataBuffer[0], HID_INT_EP_SIZE);

            /* Re-arm the OUT endpoint */
            USBHIDOutHandle = HIDRxPacket(HID_EP, (BYTE*)&RxDataBuffer, HID_INT_EP_SIZE);
        }

#ifdef ADD_CDC
	/* if our PC2PIC buffer is empty, try to re-fill it */
	if (0 == (flags & FLAG_PC2PIC_DATA_RDY))
	{
		PC2PIC_buffer_occupancy = getsUSBUSART(PC2PIC_Buffer, sizeof(PC2PIC_Buffer));
		if(PC2PIC_buffer_occupancy > 0)
		{
			/* signal that the buffer is not empty */
			flags |= FLAG_PC2PIC_DATA_RDY;
			/* rewind read index to the beginning of the buffer */
			PC2PIC_read_index = 0;
		}
	}

	/* if our PC2PIC buffer is not empty *AND* the USART can accept another byte, transmit another byte */
	if ((flags & FLAG_PC2PIC_DATA_RDY) && TXSTAbits.TRMT)
	{
		/* disable tri-state on TX transmit pin */
	        TRISCbits.TRISC4 = 0;
		/* transmit another byte from buffer */
		TXREG = PC2PIC_Buffer[PC2PIC_read_index];
		/* update the read index */
    		++PC2PIC_read_index;
		/* if the read_index has reached the occupancy value, signal that the buffer is empty again */
    		if (PC2PIC_read_index == PC2PIC_buffer_occupancy)
    			flags &= ~FLAG_PC2PIC_DATA_RDY;	    
	}

	/* if the USART has received another byte, add it to the queue if there is room */
	if (PIR1bits.RCIF && (PIC2PC_pending_count < (CDC_DATA_OUT_EP_SIZE - 1)))
	{
		if (RCSTAbits.OERR)
			RCSTAbits.CREN = 0;  /* in case of overrun error, reset the port */
		PIC2PC_Buffer[PIC2PC_pending_count] = RCREG;
		RCSTAbits.CREN = 1;  /* and then (re-)enable receive */
		++PIC2PC_pending_count;
	}

	/* if we have PIC2PC data to send and the USB stack can accept it, we hand it over */
	if ((USBUSARTIsTxTrfReady()) && (PIC2PC_pending_count > 0))
	{
		putUSBUSART(&PIC2PC_Buffer[0], PIC2PC_pending_count);
		PIC2PC_pending_count = 0;
	}

        CDCTxService();
#endif
    }
}

BOOL USER_USB_CALLBACK_EVENT_HANDLER(int event, void *pdata, WORD size)
{
    /*
    if-statement, rather than switch(), is used here as Microchip's compiler can be very inefficient with switch()
    */
    if (EVENT_CONFIGURED == event)
    {
#ifdef ADD_CDC
	/*
	for CDC
	*/
        CDCInitEP();
#endif

	/*
	for HID
	*/
        /* enable the HID endpoint */
        USBEnableEndpoint(HID_EP, USB_IN_ENABLED | USB_OUT_ENABLED | USB_HANDSHAKE_ENABLED | USB_DISALLOW_SETUP);
        /* Re-arm the OUT endpoint for the next packet */
        USBHIDOutHandle = HIDRxPacket(HID_EP, (BYTE*)&RxDataBuffer, HID_INT_EP_SIZE);
    }
    else if (EVENT_EP0_REQUEST == event)
    {
#ifdef ADD_CDC
	/*
	for CDC
	*/
        USBCheckCDCRequest();
#endif
	/*
	for HID
	*/
        USBCheckHIDRequest();
    }
    return TRUE; 
}

#ifdef ADD_CDC
void InitializeUSART(void)
{
	/* RX on RC5 is input */
        TRISCbits.TRISC5 = 1;

	/*
	TX on RC4 would be an output, but we tri-state it at boot;
	that way, we avoid actively driving it until necessary
	*/
        TRISCbits.TRISC4 = 1;

        TXSTA = 0x24;
        RCSTA = 0x90;

        /*
	load user-defined UART rate
	*/
	
	PMCON1bits.CFGS = TRUE; /* read config, not program, memory */
	PMADRH = 0;
	PMADRL = 0;
	PMCON1bits.RD = TRUE;
	/* mandatory two nops */
	_nop(); _nop();
	/* store result in baud rate registers */
        SPBRG = PMDATL;
        SPBRGH = PMDATH;

        BAUDCON = 0x08;

	/* clear any data in receive buffer */
        (volatile void)RCREG;
}

void mySetLineCodingHandler(void)
{
        CDCSetBaudRate(cdc_notice.GetLineCoding.dwDTERate.Val);
}
#endif
