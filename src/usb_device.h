#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <stdint.h>
#include <zephyr/usb/usbd.h>

struct usbd_context *usb_device_init(usbd_msg_cb_t msg_cb); // configure and initalize USB

#endif