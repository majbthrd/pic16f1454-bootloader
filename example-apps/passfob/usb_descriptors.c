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

/* 
silly Microchip USB stack convention; C files should righfully NOT be coded this way
"usb_function_hid.h" has an #ifdef that depends specifically on "__USB_DESCRIPTORS_C" to work
*/

#ifndef __USB_DESCRIPTORS_C
#define __USB_DESCRIPTORS_C

#include "./USB/usb.h"
#include "./USB/usb_function_hid.h"

/* Device Descriptor */
ROM USB_DEVICE_DESCRIPTOR device_dsc=
{
    0x12,                   // Size of this descriptor in bytes
    USB_DESCRIPTOR_DEVICE,  // DEVICE descriptor type
    0x0200,                 // USB Spec Release Number in BCD format
    0x00,                   // Class Code
    0x00,                   // Subclass code
    0x00,                   // Protocol code
    USB_EP0_BUFF_SIZE,      // Max packet size for EP0, see usb_config.h
    MY_VID,                 // Vendor ID
    MY_PID,                 // Product ID: Keyboard fw demo
    0x0001,                 // Device release number in BCD format
    0x01,                   // Manufacturer string index
    0x02,                   // Product string index
    0x00,                   // Device serial number string index
    0x01                    // Number of possible configurations
};

/* Configuration 1 Descriptor */
ROM BYTE configDescriptor1[]=
{
    /* Configuration Descriptor */
    0x09,                         // Size of this descriptor in bytes
    USB_DESCRIPTOR_CONFIGURATION, // CONFIGURATION descriptor type
    DESC_CONFIG_WORD(0x0029),     // Total length of data for this cfg
    1,                            // Number of interfaces in this cfg
    1,                            // Index value of this configuration
    0,                            // Configuration string index
    _DEFAULT,                     // Attributes, see usb_device.h
    50,                           // Max power consumption (2X mA)

    /* Interface Descriptor */
    0x09,                         // Size of this descriptor in bytes
    USB_DESCRIPTOR_INTERFACE,     // INTERFACE descriptor type
    0,                            // Interface Number
    0,                            // Alternate Setting Number
    2,                            // Number of endpoints in this intf
    HID_INTF,                     // Class code
    BOOT_INTF_SUBCLASS,           // Subclass code
    HID_PROTOCOL_KEYBOARD,        // Protocol code
    0,                            // Interface string index

    /* HID Class-Specific Descriptor */
    0x09,                             // Size of this descriptor in bytes RRoj hack
    DSC_HID,                          // HID descriptor type
    DESC_CONFIG_WORD(0x0111),         // HID Spec Release Number in BCD format (1.11)
    0x00,                             // Country Code (0x00 for Not supported)
    HID_NUM_OF_DSC,                   // Number of class descriptors, see usbcfg.h
    DSC_RPT,                          // Report descriptor type
    DESC_CONFIG_WORD(HID_RPT01_SIZE), // Size of the report descriptor
    
    /* Endpoint Descriptor */
    0x07,
    USB_DESCRIPTOR_ENDPOINT,      // Endpoint Descriptor
    HID_EP | _EP_IN,              // EndpointAddress
    _INTERRUPT,                   // Attributes
    DESC_CONFIG_WORD(8),          // size
    0x01,                         // Interval

    /* Endpoint Descriptor */
    0x07,
    USB_DESCRIPTOR_ENDPOINT,      // Endpoint Descriptor
    HID_EP | _EP_OUT,             // EndpointAddress
    _INTERRUPT,                   // Attributes
    DESC_CONFIG_WORD(8),          // size
    0x01,                         // Interval
};


static ROM struct
{
    BYTE bLength;
    BYTE bDscType;
    WORD string[1];
} sd000 =
{
    sizeof(sd000),
    USB_DESCRIPTOR_STRING,
    {0x0409}
};

static ROM struct
{
    BYTE bLength;
    BYTE bDscType;
    WORD string[4];
} sd001 =
{
    sizeof(sd001),
    USB_DESCRIPTOR_STRING,
    {'A','c','m','e'}
};

static ROM struct
{
    BYTE bLength;
    BYTE bDscType;
    WORD string[8];
} sd002 =
{
    sizeof(sd002),
    USB_DESCRIPTOR_STRING,
    {'k','e','y','b','o','a','r','d'}
};


ROM struct
{
    BYTE report[HID_RPT01_SIZE];
} hid_rpt01 = 
{
    {
        0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
        0x09, 0x06,                    // USAGE (Keyboard)
        0xa1, 0x01,                    // COLLECTION (Application)
        0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
        0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
        0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
        0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
        0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
        0x75, 0x01,                    //   REPORT_SIZE (1)
        0x95, 0x08,                    //   REPORT_COUNT (8)
        0x81, 0x02,                    //   INPUT (Data,Var,Abs)
        0x95, 0x01,                    //   REPORT_COUNT (1)
        0x75, 0x08,                    //   REPORT_SIZE (8)
        0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
        0x95, 0x05,                    //   REPORT_COUNT (5)
        0x75, 0x01,                    //   REPORT_SIZE (1)
        0x05, 0x08,                    //   USAGE_PAGE (LEDs)
        0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
        0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
        0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
        0x95, 0x01,                    //   REPORT_COUNT (1)
        0x75, 0x03,                    //   REPORT_SIZE (3)
        0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs)
        0x95, 0x06,                    //   REPORT_COUNT (6)
        0x75, 0x08,                    //   REPORT_SIZE (8)
        0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
        0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
        0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
        0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
        0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
        0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
        0xc0                           // End Collection
    }
};

ROM BYTE *ROM USB_CD_Ptr[]=
{
    (ROM BYTE *ROM)&configDescriptor1
};

ROM BYTE *ROM USB_SD_Ptr[]=
{
    (ROM BYTE *ROM)&sd000,
    (ROM BYTE *ROM)&sd001,
    (ROM BYTE *ROM)&sd002
};

#endif
