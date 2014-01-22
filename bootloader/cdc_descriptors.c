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

/*
    you MUST modify the USB descriptors and VID:PID below to suit your application
*/

/* 
silly Microchip USB stack convention; C files should righfully NOT be coded this way
"usb_function_hid.h" has an #ifdef that depends specifically on "__USB_DESCRIPTORS_C" to work
*/
#ifndef __USB_DESCRIPTORS_C
#define __USB_DESCRIPTORS_C

#include "./USB/usb.h"
#ifdef ADD_CDC
#include "./USB/usb_function_cdc.h"
#endif
#include "./USB/usb_function_hid.h"

/* Device Descriptor for bootloader */
ROM USB_DEVICE_DESCRIPTOR bootloader_device_dsc=
{
    0x12,                   // Size of this descriptor in bytes
    USB_DESCRIPTOR_DEVICE,  // DEVICE descriptor type
    0x0200,                 // USB Spec Release Number in BCD format
    0x00,                   // Class Code
    0x00,                   // Subclass code
    0x00,                   // Protocol code
    USB_EP0_BUFF_SIZE,      // Max packet size for EP0, see usb_config.h
    0x04D8,                 // Vendor ID
    0x5A5A,                 // Product ID
    0x0100,                 // Device release number in BCD format
    0x01,                   // Manufacturer string index
    0x02,                   // Product string index
    0x00,                   // Device serial number string index
    0x01                    // Number of possible configurations
};

ROM BYTE configDescriptor1[]={
    /* Configuration Descriptor */
    0x09,                   // Size of this descriptor in bytes
    USB_DESCRIPTOR_CONFIGURATION,// CONFIGURATION descriptor type
#ifdef ADD_CDC
    107,0,                  // Total length of data for this cfg
#else
    41,0,                   // Total length of data for this cfg
#endif
    3,                      // Number of interfaces in this cfg
    1,                      // Index value of this configuration
    0,                      // Configuration string index
    _DEFAULT,               // Attributes, see usb_device.h
    50,                     // Max power consumption (2X mA)
							
    /* Interface Descriptor */
    0x09,                   // Size of this descriptor in bytes
    USB_DESCRIPTOR_INTERFACE,// INTERFACE descriptor type
    HID_INTF_ID,            // Interface Number
    0,                      // Alternate Setting Number
    2,                      // Number of endpoints in this intf
    HID_INTF,               // Class code
    0,                      // Subclass code
    0,                      // Protocol code
    0,                      // Interface string index
    
    /* HID Class-Specific Descriptor */
    0x09,                   // Size of this descriptor in bytes
    DSC_HID,                // HID descriptor type
    0x00,0x01,              // HID Spec Release Number in BCD format (1.00)
    0x00,                   // Country Code (0x00 for Not supported)
    HID_NUM_OF_DSC,         // Number of class descriptors, see usbcfg.h
    DSC_RPT,                // Report descriptor type
    HID_RPT01_SIZE,0x00,    // Size of the report descriptor

    /* Endpoint Descriptor */
    0x07,
    USB_DESCRIPTOR_ENDPOINT,    //Endpoint Descriptor
    HID_EP | _EP_IN,            //EndpointAddress
    _INTERRUPT,                 //Attributes
    0x40,0x00,                  //size
    0x01,                       //Interval

    /* Endpoint Descriptor */
    0x07,
    USB_DESCRIPTOR_ENDPOINT,    //Endpoint Descriptor
    HID_EP | _EP_OUT,           //EndpointAddress
    _INTERRUPT,                 //Attributes
    0x40,0x00,                  //size
    0x01,                       //Interval

#ifdef ADD_CDC
    /* Interface Association Descriptor */    
    0x08,                   // Size of this descriptor in byte    
    0x0B,                   // Interface assocication descriptor type    
    1,                      // The first associated interface    
    2,                      // Number of contiguous associated interface    
    COMM_INTF,              // bInterfaceClass of the first interface    
    ABSTRACT_CONTROL_MODEL, // bInterfaceSubclass of the first interface    
    V25TER,                 // bInterfaceProtocol of the first interface    
    0,                      // Interface string index   

    /* Interface Descriptor */
    9,                        // Size of this descriptor in bytes
    USB_DESCRIPTOR_INTERFACE, // INTERFACE descriptor type
    CDC_COMM_INTF_ID,         // Interface Number
    0,                        // Alternate Setting Number
    1,                        // Number of endpoints in this intf
    COMM_INTF,                // Class code
    ABSTRACT_CONTROL_MODEL,   // Subclass code
    V25TER,                   // Protocol code
    0,                        // Interface string index

    /* CDC Class-Specific Descriptors */
    sizeof(USB_CDC_HEADER_FN_DSC),
    CS_INTERFACE,
    DSC_FN_HEADER,
    0x10,0x01,

    sizeof(USB_CDC_ACM_FN_DSC),
    CS_INTERFACE,
    DSC_FN_ACM,
    USB_CDC_ACM_FN_DSC_VAL,

    sizeof(USB_CDC_UNION_FN_DSC),
    CS_INTERFACE,
    DSC_FN_UNION,
    CDC_COMM_INTF_ID,
    CDC_DATA_INTF_ID,

    sizeof(USB_CDC_CALL_MGT_FN_DSC),
    CS_INTERFACE,
    DSC_FN_CALL_MGT,
    0x00,
    CDC_DATA_INTF_ID,

    /* Endpoint Descriptor */
    0x07,
    USB_DESCRIPTOR_ENDPOINT,    //Endpoint Descriptor
    CDC_COMM_EP | _EP_IN,       //EndpointAddress
    _INTERRUPT,                 //Attributes
    0x08,0x00,                  //size
    0x02,                       //Interval

    /* Interface Descriptor */
    9,                      // Size of this descriptor in bytes
    USB_DESCRIPTOR_INTERFACE,// INTERFACE descriptor type
    CDC_DATA_INTF_ID,       // Interface Number
    0,                      // Alternate Setting Number
    2,                      // Number of endpoints in this intf
    DATA_INTF,              // Class code
    0,                      // Subclass code
    NO_PROTOCOL,            // Protocol code
    0,                      // Interface string index
    
    /* Endpoint Descriptor */
    0x07,
    USB_DESCRIPTOR_ENDPOINT,    //Endpoint Descriptor
    CDC_DATA_EP | _EP_OUT,      //EndpointAddress
    _BULK,                      //Attributes
    0x40,0x00,                  //size
    0x00,                       //Interval

    /* Endpoint Descriptor */
    0x07,
    USB_DESCRIPTOR_ENDPOINT,   //Endpoint Descriptor
    CDC_DATA_EP | _EP_IN,      //EndpointAddress
    _BULK,                     //Attributes
    0x40,0x00,                 //size
    0x00,                      //Interval
#endif
};

/*
It would be desirable to re-use the same structures sd000 and sd001 in hidplus_descriptors.c.
However, for reasons unknown, the code goes into the weeds if either:
a) the xxx_descriptors.c files are combined into one
b) two _descriptors.c files are maintained, but share common sd000 and sd001 structs
*/

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

#ifdef ADD_CDC
static ROM struct
{
    BYTE bLength;
    BYTE bDscType;
    WORD string[3];
} sd004 =
{
    sizeof(sd004),
    USB_DESCRIPTOR_STRING,
    {'C','D','C'}
};
#else
static ROM struct
{
    BYTE bLength;
    BYTE bDscType;
    WORD string[10];
} sd004 =
{
    sizeof(sd004),
    USB_DESCRIPTOR_STRING,
    {'b','o','o','t','l','o','a','d','e','r'}
};
#endif

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
        0x25, 0x40,      	//      Logical Maximum (data bytes in the report may have maximum value = 0x00FF = unsigned 255)
        0x75, 0x08,             //      Report Size: 8-bit field size
        0x95, 0x40,             //      Report Count: Make sixty-four 8-bit fields (the next time the parser hits an "Input", "Output", or "Feature" item)
        0x81, 0x00,             //      Input (Data, Array, Abs): Instantiates input packet fields based on the above report size, count, logical min/max, and usage.
        0x19, 0x01,             //      Usage Minimum 
        0x29, 0x40,             //      Usage Maximum 	//64 output usages total (0x01 to 0x40)
        0x91, 0x00,             //      Output (Data, Array, Abs): Instantiates output packet fields.  Uses same report size and count as "Input" fields, since nothing new/different was specified to the parser since the "Input" item.
        0xC0
    }
};                  

ROM BYTE *ROM CDC_USB_CD_Ptr[]=
{
    (ROM BYTE *ROM)&configDescriptor1
};

ROM BYTE *ROM CDC_USB_SD_Ptr[USB_NUM_STRING_DESCRIPTORS]=
{
    (ROM BYTE *ROM)&sd000,
    (ROM BYTE *ROM)&sd001,
    (ROM BYTE *ROM)&sd004
};

#endif
