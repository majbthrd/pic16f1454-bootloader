/*
    this variant of code is based on:

    M-Stack USB Device Stack Implementation
    Copyright (C) 2013 Alan Ott <alan@signal11.us>
    Copyright (C) 2013 Signal 11 Software

    rather than Microchip's USB Framework
*/

/*
    USB HID bootloader for PIC16F1454/PIC16F1455/PIC16F1459 microcontroller

    Copyright (C) 2013,2014,2015 Peter Lawrence

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
#include "usb_hid.h"

/* state of configuration words */
__CONFIG(FOSC_INTOSC & WDTE_SWDTEN & PWRTE_ON & MCLRE_OFF & CP_ON & BOREN_ON & CLKOUTEN_OFF & IESO_OFF & FCMEN_OFF);
__CONFIG(WRT_HALF & CPUDIV_NOCLKDIV & USBLSCLK_48MHz & PLLMULT_3x & PLLEN_ENABLED & STVREN_ON & BORV_LO & LPBOR_OFF & LVP_OFF);

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
  uint16_t crc, data;
  uint8_t index, lo, hi;
  uint8_t tx_count;
  uint8_t flags;
  uint8_t *TxDataBuffer;
  const uint8_t *RxDataBuffer;
  uint8_t len;

  /* enable pull-up on RA3 (for boot mode detection) */
  OPTION_REGbits.nWPUEN = 0;
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
    PMCON1bits.CFGS = 0;
    /* set RD (initiate read) */
    PMCON1bits.RD = 1;
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
  OPTION_REGbits.nWPUEN = 1;
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

  usb_init();

  TxDataBuffer = usb_get_in_buffer(1);

  for (;;)
  {
      usb_service();

      /* if USB isn't configured, there is no point in proceeding further */
      if (!usb_is_configured())
         continue;

      /*
      we check these *BEFORE* calling usb_out_endpoint_has_data() as the documentation indicates this 
      must be followed usb_arm_out_endpoint() to enable reception of the next transaction
      */
      if (usb_in_endpoint_halted(1) || usb_in_endpoint_busy(1))
         continue;

      /* if we pass this test, we are committed to make the usb_arm_out_endpoint() call */
      if (!usb_out_endpoint_has_data(1))
         continue;

      /* obtain a pointer to the receive buffer and the length of data contained within it */
      len = usb_get_out_buffer(1, &RxDataBuffer);

      /*
      if tx_count is set to be non-zero by subsequent code, this indicates data
      of length tx_count is in DataBuffer[] to be passed to the USB driver
      */
      tx_count = 0;

      /*
      parse incoming HID packet and generate response
      */

      TxDataBuffer[tx_count++] = RxDataBuffer[0];
      TxDataBuffer[tx_count++] = RxDataBuffer[1];
      TxDataBuffer[tx_count++] = RxDataBuffer[2];

      hi = RxDataBuffer[1];
      lo = RxDataBuffer[2];

      /* clear CFGS for accessing Program Memory; set accessing Configuration Memory */
      PMCON1bits.CFGS = (RxDataBuffer[0] & 0x04) ? 1 : 0;

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
            PMCON1bits.RD = 1;
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
          PMCON1bits.FREE = 1;
          /* enable write/erase operation */
          PMCON1bits.WREN = 1;
          /* unlock sequence */
          PMCON2 = 0x55;
          PMCON2 = 0xAA;
          PMCON1bits.WR = 1;
          /* mandatory two nops */
          _nop(); _nop();
          /* disable write/erase operation */
          PMCON1bits.WREN = 0;
          break;

      case 0x82:  /* Program Memory */
      case 0x86:  /* Program Config */
          /* provide Program Memory row address */
          PMADRH = hi;
          PMADRL = lo;
          /* select write operation */
          PMCON1bits.FREE = 0;
          /* load write latches only */
          PMCON1bits.LWLO = 1;
          /* enable write/erase operation */
          PMCON1bits.WREN = 1;

          index = 3;

          for (;;)
          {
              PMDATH = RxDataBuffer[index++];
              PMDATL = RxDataBuffer[index++];
              if ( (index >= (32 + 3)) || PMCON1bits.CFGS )
              {
                  /* write latches to flash */
                  PMCON1bits.LWLO = 0;
              }
              /* unlock sequence */
              PMCON2 = 0x55;
              PMCON2 = 0xAA;
              PMCON1bits.WR = 1;
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
          PMCON1bits.WREN = 0;
          break;
      }

      usb_send_in_buffer(1, EP_1_IN_LEN);

      /* re-arm the endpoint to receive the next EP1 OUT */
      usb_arm_out_endpoint(1);
   }
}

/*
Signal 11 M-Stack Callbacks
*/

void app_set_configuration_callback(uint8_t configuration) {}

uint16_t app_get_device_status_callback()
{
   return 0x0000;
}

void app_endpoint_halt_callback(uint8_t endpoint, bool halted) {}

int8_t app_set_interface_callback(uint8_t interface, uint8_t alt_setting)
{
   return 0;
}

int8_t app_get_interface_callback(uint8_t interface)
{
   return 0;
}

void app_out_transaction_callback(uint8_t endpoint) {}

void app_in_transaction_complete_callback(uint8_t endpoint) {}

int8_t app_unknown_setup_request_callback(const struct setup_packet *setup)
{
   return process_hid_setup_request(setup);
}

int16_t app_unknown_get_descriptor_callback(const struct setup_packet *pkt, const void **descriptor)
{
   return -1;
}

void app_start_of_frame_callback(void) {}

void app_usb_reset_callback(void) {}
