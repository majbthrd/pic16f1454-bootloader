The PIC16F1454 microcontroller is an aggressively priced (under $1.50 in single quantity and under $1.00 in larger quantities) USB-enabled microcontroller from Microchip. It is also able to discipline its internal RC oscillator using the USB interface, allowing the designer to forgo an external crystal (further enhancing the cost advantage over other vendors' microcontrollers).

No example bootloader source code was provided by Microchip for this device family. I had to write my own, and this project shares that C source code.

The (legacy) C source code for this bootloader uses the first 4096 words (half) of flash memory due to the available hardware options.

However, there is now a [new 512 word bootloader](https://github.com/majbthrd/PIC16F1-USB-DFU-Bootloader) that is written in assembly code and uses only 512 words, leaving 93.7% of the flash memory available for the user.

The C code bootloader of this legacy project operates as a vendor-defined HID USB device, meaning it is compatible with multiple OSes and requires no drivers.  The [new bootloader](https://github.com/majbthrd/PIC16F1-USB-DFU-Bootloader) operates as a DFU USB device, also making it compatible with multiple OSes and using existing download software.

This project now also includes several example USB applications written for the PIC16F1454 using the XC8 compiler; see "example-apps" directory and Wiki for details.

