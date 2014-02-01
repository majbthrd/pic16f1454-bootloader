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
    MY_PID,                 // Product ID: Mouse in a circle fw demo
    0x0003,                 // Device release number in BCD format
    0x01,                   // Manufacturer string index
    0x02,                   // Product string index
    0x00,                   // Device serial number string index
    0x01                    // Number of possible configurations
};

/* Configuration 1 Descriptor */
ROM BYTE configDescriptor1[]={
    /* Configuration Descriptor */
    0x09,                         // Size of this descriptor in bytes
    USB_DESCRIPTOR_CONFIGURATION, // CONFIGURATION descriptor type
    DESC_CONFIG_WORD(0x0022),     // Total length of data for this cfg
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
    1,                            // Number of endpoints in this intf
    HID_INTF,                     // Class code
    BOOT_INTF_SUBCLASS,           // Subclass code
    HID_PROTOCOL_MOUSE,           // Protocol code
    0,                            // Interface string index

    /* HID Class-Specific Descriptor */
    0x09,                         // Size of this descriptor in bytes RRoj hack
    DSC_HID,                      // HID descriptor type
    DESC_CONFIG_WORD(0x0111),     // HID Spec Release Number in BCD format (1.11)
    0x00,                         // Country Code (0x00 for Not supported)
    HID_NUM_OF_DSC,               // Number of class descriptors, see usbcfg.h
    DSC_RPT,                      // Report descriptor type
    DESC_CONFIG_WORD(50),         // Size of the report descriptor
    
    /* Endpoint Descriptor */
    0x07,
    USB_DESCRIPTOR_ENDPOINT,      // Endpoint Descriptor
    HID_EP | _EP_IN,              // EndpointAddress
    _INTERRUPT,                   // Attributes
    DESC_CONFIG_WORD(3),          // size
    0x01                          // Interval
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
    WORD string[5];
} sd002 =
{
    sizeof(sd002),
    USB_DESCRIPTOR_STRING,
    {'m','o','u','s','e'}
};

ROM struct
{
    BYTE report[HID_RPT01_SIZE];
} hid_rpt01 = 
{
    {
        0x05, 0x01, /* Usage Page (Generic Desktop)             */
        0x09, 0x02, /* Usage (Mouse)                            */
        0xA1, 0x01, /* Collection (Application)                 */
        0x09, 0x01, /*  Usage (Pointer)                         */
        0xA1, 0x00, /*  Collection (Physical)                   */
        0x05, 0x09, /*      Usage Page (Buttons)                */
        0x19, 0x01, /*      Usage Minimum (01)                  */
        0x29, 0x03, /*      Usage Maximum (03)                  */
        0x15, 0x00, /*      Logical Minimum (0)                 */
        0x25, 0x01, /*      Logical Maximum (1)                 */
        0x95, 0x03, /*      Report Count (3)                    */
        0x75, 0x01, /*      Report Size (1)                     */
        0x81, 0x02, /*      Input (Data, Variable, Absolute)    */
        0x95, 0x01, /*      Report Count (1)                    */
        0x75, 0x05, /*      Report Size (5)                     */
        0x81, 0x01, /*      Input (Constant)    ;5 bit padding  */
        0x05, 0x01, /*      Usage Page (Generic Desktop)        */
        0x09, 0x30, /*      Usage (X)                           */
        0x09, 0x31, /*      Usage (Y)                           */
        0x15, 0x81, /*      Logical Minimum (-127)              */
        0x25, 0x7F, /*      Logical Maximum (127)               */
        0x75, 0x08, /*      Report Size (8)                     */
        0x95, 0x02, /*      Report Count (2)                    */
        0x81, 0x06, /*      Input (Data, Variable, Relative)    */
        0xC0, 0xC0
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
