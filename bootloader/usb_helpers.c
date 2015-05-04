#include "usb.h"
#include <xc.h>
#include "usb_config.h"
#include "usb_ch9.h"
#include "usb_hid.h"


/*
Signal 11 M-Stack Callbacks
*/

uint16_t app_get_device_status_callback()
{
	return 0x0000;
}

int8_t app_set_interface_callback(uint8_t interface, uint8_t alt_setting)
{
	return 0;
}

int8_t app_get_interface_callback(uint8_t interface)
{
	return 0;
}

int8_t app_unknown_setup_request_callback(const struct setup_packet *setup)
{
	return process_hid_setup_request(setup);
}

int16_t app_unknown_get_descriptor_callback(const struct setup_packet *pkt, const void **descriptor)
{
	return -1;
}
