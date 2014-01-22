/*
   Microchip's descriptor copying routines either work or cause the CPU to go into the weeds
   manually inserting (and wastefully throwing away precious flash) dummy data keeps it on the rails
*/

#include "./USB/usb.h"

ROM BYTE foo[70];
