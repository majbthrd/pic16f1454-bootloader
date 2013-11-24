/*
    USB HID bootloader for PIC16F1454/PIC16F1455/PIC16F1459 microcontroller

    Copyright (C) 2013 Peter Lawrence

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

/* state of configuration words */
__CONFIG(FOSC_INTOSC & WDTE_OFF & PWRTE_ON & MCLRE_OFF & CP_ON & BOREN_ON & CLKOUTEN_OFF & IESO_OFF & FCMEN_OFF);
__CONFIG(WRT_HALF & CPUDIV_NOCLKDIV & USBLSCLK_48MHz & PLLMULT_3x & PLLEN_ENABLED & STVREN_ON & BORV_LO & LPBOR_OFF & LVP_OFF);

/* USB data buffers (which are also available to the user code when in HIDHYBRID boot mode) */
unsigned char RxDataBuffer[64] @0xA0;
unsigned char TxDataBuffer[64] @0x120;

/* handles used for HID functionality */
USB_HANDLE USBOutHandle;
USB_HANDLE USBInHandle;

/* prototyping of external USB descriptors in ROM */
extern ROM USB_DEVICE_DESCRIPTOR hid_bootloader_device_dsc;
extern ROM BYTE *ROM HID_USB_CD_Ptr[];
extern ROM BYTE *ROM HID_USB_SD_Bootloader_Ptr[];
#ifndef OMIT_HIDHYBRID
extern ROM USB_DEVICE_DESCRIPTOR hid_cypher_device_dsc;
extern ROM BYTE *ROM HID_USB_SD_Cypher_Ptr[];
#endif

/* pointers that pick one of the above external USB descriptors */
USB_DEVICE_DESCRIPTOR const *pnt_device_dsc;
const BYTE const **USB_CD_Ptr;
const BYTE const **USB_SD_Ptr;

/* variable storing boot mode (which determines the code's behavior) */
static enum
{
    BOOTLOADER,
    USERIMAGE,
#ifndef OMIT_HIDHYBRID
    HIDHYBRID
#endif
} BootMode = BOOTLOADER;

#ifndef OMIT_HIDHYBRID
/* flag from USER_USB_CALLBACK_EVENT_HANDLER() to main loop */
static BOOL msTick = FALSE;

/* XC8 syntax approach to optionally call (not goto) HIDHYBRID user code */
extern BYTE usercode(BYTE) @ 0x1000;
#endif

void interrupt ISRCode()
{
    /*
    Microchip's ever varying nomenclature is as follows:
    the goto (no return address pushed onto stack) is called "ljmp" ( *LONG* jump )
    but the call (return address pushed onto stack) is called "fcall" ( *FAR* call )
    */
    if (USERIMAGE == BootMode)
    {
        /* in USERIMAGE boot mode, the user's code will handle returning from the interrupt */
        asm("ljmp 0x1004"); /* call user code interrupt vector */
    }
    else
    {
        /* in BOOTLOADER and HIDHYBRID boot modes, this is the complete interrupt handler */
        USBDeviceTasks();
    }
}

int main(void)
{
    WORD crc, data;
    BYTE index, lo, hi;
    BYTE tx_count;
    
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
            if (0x20 == hi)
                break;
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
        if (data == crc)
            BootMode = USERIMAGE;
#ifndef OMIT_HIDHYBRID
        else if ((data ^ 0x3FFF) == crc)
            BootMode = HIDHYBRID;
#endif
    }

    /* disable pull-up now that determination has been made */
    OPTION_REGbits.nWPUEN = TRUE;
    WPUA = 0;

    /* if not in bootloader mode, branch to user code (for initialization or outright different USB code) */
    if (USERIMAGE == BootMode)
    {
        /*
        in USERIMAGE boot mode, we goto ("LONG jump") the user's code with no expectation of returning
        doing a call would function, but would waste one entry on the stack
        */
        asm("ljmp 0x1000"); /* call user code reset vector */
    }
#ifndef OMIT_HIDHYBRID
    else if (HIDHYBRID == BootMode)
    {
        /*
        in HIDHYBRID mode, we expect the user-provided code to return
        */
        usercode(2);
    }
#endif

    /* choose USB descriptor based on BootMode */
    pnt_device_dsc = &hid_bootloader_device_dsc;
    USB_CD_Ptr = (const BYTE const **)HID_USB_CD_Ptr;
    USB_SD_Ptr = (const BYTE const **)HID_USB_SD_Bootloader_Ptr;
#ifndef OMIT_HIDHYBRID
    if (HIDHYBRID == BootMode)
    {
        pnt_device_dsc = &hid_cypher_device_dsc;
        USB_SD_Ptr = (const BYTE const **)HID_USB_SD_Cypher_Ptr;
    }
#endif

    /* initialize the USB driver */
    USBOutHandle = 0;
    USBInHandle = 0;
    USBDeviceInit();
    USBDeviceAttach();

    for (;;)
    {
        // User Application USB tasks
        if( (USBDeviceState < CONFIGURED_STATE) || (1 == USBSuspendControl) )
            continue;

        //Check if we have received an OUT data packet from the host
        if (!HIDRxHandleBusy(USBOutHandle))
        {
            /*
            if tx_count is set to be non-zero by subsequent code, this indicates data
            of length tx_count is in DataBuffer[] to be passed to the USB driver
            */
            tx_count = 0;

            if (!HIDTxHandleBusy(USBInHandle))
            {
                /*
                if the code has reached here, the BootMode is either BOOTLOADER or HIDHYBRID
                here we branch accordingly (either calling bootloader code or the user's code)
                */
#ifndef OMIT_HIDHYBRID
                if (BOOTLOADER == BootMode)
#endif
                {
                    TxDataBuffer[tx_count++] = RxDataBuffer[0];
                    TxDataBuffer[tx_count++] = RxDataBuffer[1];
                    TxDataBuffer[tx_count++] = RxDataBuffer[2];

                    hi = RxDataBuffer[1];
                    lo = RxDataBuffer[2];

                    switch (RxDataBuffer[0])
                    {
                    case 0x80:  /* Read Memory */
                        do
                        {
                            /* set starting Program Memory address */
                            PMADRH = hi;
                            PMADRL = lo;
                            /* clear CFGS (we're reading Program, not Configuration, Memory) */
                            PMCON1bits.CFGS = FALSE;
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
                        /* disable global interrupts */
                        INTCONbits.GIE = FALSE;
                        /* clear CFGS (we're reading Program, not Configuration, Memory) */
                        PMCON1bits.CFGS = FALSE;
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
                        /* re-enable global interrupts */
                        INTCONbits.GIE = TRUE;
                        break;

                    case 0x82:  /* Program Memory */
                        /* disable global interrupts */
                        INTCONbits.GIE = FALSE;
                        /* clear CFGS (we're reading Program, not Configuration, Memory) */
                        PMCON1bits.CFGS = FALSE;
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
                            if (index >= (32 + 3))
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
                            if (index >= (32 + 3))
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
                        /* re-enable global interrupts */
                        INTCONbits.GIE = TRUE;
                        break;
                    }
                }
#ifndef OMIT_HIDHYBRID
                else
                {
                    /* call user-provided HIDHYBRID boot mode code */
                    tx_count = usercode(0);
                }
#endif
            }

            if (tx_count)
                USBInHandle = HIDTxPacket(HID_EP, (BYTE*)&TxDataBuffer[0], tx_count);

            /* Re-arm the OUT endpoint */
            USBOutHandle = HIDRxPacket(HID_EP, (BYTE*)&RxDataBuffer, 64);
        }

#ifndef OMIT_HIDHYBRID
        if (msTick)
        {
            if (HIDHYBRID == BootMode)
            {
                usercode(1);
            }
            msTick = FALSE;
        }
#endif
    }
}

/*******************************************************************
 * Function:        BOOL USER_USB_CALLBACK_EVENT_HANDLER(
 *                        USB_EVENT event, void *pdata, WORD size)
 *
 * PreCondition:    None
 *
 * Input:           USB_EVENT event - the type of event
 *                  void *pdata - pointer to the event data
 *                  WORD size - size of the event data
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is called from the USB stack to
 *                  notify a user application that a USB event
 *                  occured.  This callback is in interrupt context
 *                  when the USB_INTERRUPT option is selected.
 *
 * Note:            None
 *******************************************************************/
BOOL USER_USB_CALLBACK_EVENT_HANDLER(int event, void *pdata, WORD size)
{
    /*
    if-statement, rather than switch(), is used here as Microchip's compiler can be very inefficient with switch()
    */
    if (EVENT_SOF == event)
    {
#ifndef OMIT_HIDHYBRID
        /* flag main loop to service millisecond tick */
        msTick = TRUE;
#endif
    }
    else if (EVENT_CONFIGURED == event)
    {
        //enable the HID endpoint
        USBEnableEndpoint(HID_EP, USB_IN_ENABLED | USB_OUT_ENABLED | USB_HANDSHAKE_ENABLED | USB_DISALLOW_SETUP);
        //Re-arm the OUT endpoint for the next packet
        USBOutHandle = HIDRxPacket(HID_EP,(BYTE*)&RxDataBuffer,64);
    }
    else if (EVENT_EP0_REQUEST == event)
    {
        USBCheckHIDRequest();
    }
    return TRUE; 
}

