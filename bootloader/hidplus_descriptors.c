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

/*
    modify the USB descriptors below to suit your application
*/

/* silly Microchip USB stack convention; C files should righfully NOT be coded this way */
#ifndef __USB_DESCRIPTORS_C
#define __USB_DESCRIPTORS_C

#include "./USB/usb.h"
#include "./USB/usb_function_hid.h"

/* Device Descriptor for bootloader */
ROM USB_DEVICE_DESCRIPTOR hid_bootloader_device_dsc=
{
    0x12,    // Size of this descriptor in bytes
    USB_DESCRIPTOR_DEVICE,                // DEVICE descriptor type
    0x0200,                 // USB Spec Release Number in BCD format
    0x00,                   // Class Code
    0x00,                   // Subclass code
    0x00,                   // Protocol code
    USB_EP0_BUFF_SIZE,      // Max packet size for EP0, see usb_config.h
    0x04D8,                 // Vendor ID
    0x003F,                 // Product ID: Custom HID device demo
    0x0002,                 // Device release number in BCD format
    0x01,                   // Manufacturer string index
    0x02,                   // Product string index
    0x00,                   // Device serial number string index
    0x01                    // Number of possible configurations
};

#ifndef OMIT_HIDHYBRID
/* Device Descriptor for HIDHYBRID mode */
ROM USB_DEVICE_DESCRIPTOR hid_cypher_device_dsc=
{
    0x12,    // Size of this descriptor in bytes
    USB_DESCRIPTOR_DEVICE,                // DEVICE descriptor type
    0x0200,                 // USB Spec Release Number in BCD format
    0x00,                   // Class Code
    0x00,                   // Subclass code
    0x00,                   // Protocol code
    USB_EP0_BUFF_SIZE,      // Max packet size for EP0, see usb_config.h
    0x04D8,                 // Vendor ID
    0x003F,                 // Product ID: Custom HID device demo
    0x0002,                 // Device release number in BCD format
    0x01,                   // Manufacturer string index
    0x02,                   // Product string index
    0x00,                   // Device serial number string index
    0x01                    // Number of possible configurations
};
#endif

/* Configuration 1 Descriptor */
ROM BYTE configDescriptor1[]=
{
    /* Configuration Descriptor */
    0x09,//sizeof(USB_CFG_DSC),    // Size of this descriptor in bytes
    USB_DESCRIPTOR_CONFIGURATION,                // CONFIGURATION descriptor type
    0x29,0x00,            // Total length of data for this cfg
    1,                      // Number of interfaces in this cfg
    1,                      // Index value of this configuration
    0,                      // Configuration string index
    _DEFAULT,               // Attributes, see usb_device.h
    50,                     // Max power consumption (2X mA)
							
    /* Interface Descriptor */
    0x09,//sizeof(USB_INTF_DSC),   // Size of this descriptor in bytes
    USB_DESCRIPTOR_INTERFACE,               // INTERFACE descriptor type
    0,                      // Interface Number
    0,                      // Alternate Setting Number
    2,                      // Number of endpoints in this intf
    HID_INTF,               // Class code
    0,     // Subclass code
    0,     // Protocol code
    0,                      // Interface string index

    /* HID Class-Specific Descriptor */
    0x09,//sizeof(USB_HID_DSC)+3,    // Size of this descriptor in bytes
    DSC_HID,                // HID descriptor type
    0x11,0x01,                 // HID Spec Release Number in BCD format (1.11)
    0x00,                   // Country Code (0x00 for Not supported)
    HID_NUM_OF_DSC,         // Number of class descriptors, see usbcfg.h
    DSC_RPT,                // Report descriptor type
    HID_RPT01_SIZE,0x00,//sizeof(hid_rpt01),      // Size of the report descriptor
    
    /* Endpoint Descriptor */
    0x07,/*sizeof(USB_EP_DSC)*/
    USB_DESCRIPTOR_ENDPOINT,    //Endpoint Descriptor
    HID_EP | _EP_IN,                   //EndpointAddress
    _INTERRUPT,                       //Attributes
    0x40,0x00,                  //size
    0x01,                        //Interval

    /* Endpoint Descriptor */
    0x07,/*sizeof(USB_EP_DSC)*/
    USB_DESCRIPTOR_ENDPOINT,    //Endpoint Descriptor
    HID_EP | _EP_OUT,                   //EndpointAddress
    _INTERRUPT,                       //Attributes
    0x40,0x00,                  //size
    0x01                        //Interval
};

//Language code string descriptor
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

//Manufacturer string descriptor
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
    WORD string[10];
} sd002 =
{
    sizeof(sd002),
    USB_DESCRIPTOR_STRING,
    {'b','o','o','t','l','o','a','d','e','r'}
};

#ifndef OMIT_HIDHYBRID
static ROM struct
{
    BYTE bLength;
    BYTE bDscType;
    WORD string[6];
} sd003 =
{
    sizeof(sd003),
    USB_DESCRIPTOR_STRING,
    {'C','y','p','h','e','r'}
};
#endif

//Class specific descriptor - HID 
ROM struct
{
    BYTE report[HID_RPT01_SIZE];
} hid_rpt01 = 
{
    {
        0x06, 0x00, 0xFF,       // Usage Page = 0xFF00 (Vendor Defined Page 1)
        0x09, 0x01,             // Usage (Vendor Usage 1)
        0xA1, 0x01,             // Collection (Application)
        0x19, 0x01,             //      Usage Minimum 
        0x29, 0x40,             //      Usage Maximum 	//64 input usages total (0x01 to 0x40)
        0x15, 0x01,             //      Logical Minimum (data bytes in the report may have minimum value = 0x00)
        0x25, 0x40,      	  	//      Logical Maximum (data bytes in the report may have maximum value = 0x00FF = unsigned 255)
        0x75, 0x08,             //      Report Size: 8-bit field size
        0x95, 0x40,             //      Report Count: Make sixty-four 8-bit fields (the next time the parser hits an "Input", "Output", or "Feature" item)
        0x81, 0x00,             //      Input (Data, Array, Abs): Instantiates input packet fields based on the above report size, count, logical min/max, and usage.
        0x19, 0x01,             //      Usage Minimum 
        0x29, 0x40,             //      Usage Maximum 	//64 output usages total (0x01 to 0x40)
        0x91, 0x00,             //      Output (Data, Array, Abs): Instantiates output packet fields.  Uses same report size and count as "Input" fields, since nothing new/different was specified to the parser since the "Input" item.
        0xC0
    }
};                  

ROM BYTE *ROM HID_USB_CD_Ptr[]=
{
    (ROM BYTE *ROM)&configDescriptor1
};

ROM BYTE *ROM HID_USB_SD_Bootloader_Ptr[USB_NUM_STRING_DESCRIPTORS] =
{
    (ROM BYTE *ROM)&sd000,
    (ROM BYTE *ROM)&sd001,
    (ROM BYTE *ROM)&sd002
};

#ifndef OMIT_HIDHYBRID
ROM BYTE *ROM HID_USB_SD_Cypher_Ptr[USB_NUM_STRING_DESCRIPTORS] =
{
    (ROM BYTE *ROM)&sd000,
    (ROM BYTE *ROM)&sd001,
    (ROM BYTE *ROM)&sd003
};
#endif

#endif
