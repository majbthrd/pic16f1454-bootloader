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

#ifndef USBCFG_H
#define USBCFG_H

#define USB_EP0_BUFF_SIZE	8
#define USB_MAX_NUM_INT     	3
#define USB_MAX_EP_NUMBER	3

#define USB_USER_DEVICE_DESCRIPTOR pnt_device_dsc
#define USB_USER_DEVICE_DESCRIPTOR_INCLUDE extern ROM USB_DEVICE_DESCRIPTOR *pnt_device_dsc

#define USB_USER_CONFIG_DESCRIPTOR USB_CD_Ptr
#define USB_USER_CONFIG_DESCRIPTOR_INCLUDE extern const BYTE const **USB_CD_Ptr


#define USB_PING_PONG_MODE USB_PING_PONG__FULL_PING_PONG

#define USB_POLLING

#define USB_PULLUP_OPTION USB_PULLUP_ENABLE

#define USB_TRANSCEIVER_OPTION USB_INTERNAL_TRANSCEIVER

#define USB_SPEED_OPTION USB_FULL_SPEED

#define USB_ENABLE_STATUS_STAGE_TIMEOUTS

#define USB_STATUS_STAGE_TIMEOUT     (BYTE) 45 /* ms */

#define USB_SUPPORT_DEVICE

#define USB_NUM_STRING_DESCRIPTORS 3

#define USB_ENABLE_ALL_HANDLERS

#define USB_USE_HID

/* HID endpoints */
#define HID_INTF_ID             0x00
#define HID_EP 			1
#define HID_INT_EP_SIZE         64
#define HID_NUM_OF_DSC          1
#define HID_RPT01_SIZE          28

#ifdef ADD_CDC
#define USB_USE_CDC

/* CDC endpoints */
#define CDC_COMM_INTF_ID        0x01
#define CDC_COMM_EP             2
#define CDC_COMM_IN_EP_SIZE     10

#define CDC_DATA_INTF_ID        0x02
#define CDC_DATA_EP             3
#define CDC_DATA_OUT_EP_SIZE    64
#define CDC_DATA_IN_EP_SIZE     64

#define USB_CDC_SET_LINE_CODING_HANDLER mySetLineCodingHandler
#define USB_CDC_SUPPORT_ABSTRACT_CONTROL_MANAGEMENT_CAPABILITIES_D1
#endif

#endif //USBCFG_H
