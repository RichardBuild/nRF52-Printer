#ifndef USB_MSC_H
#define USB_MSC_H

#include <stdint.h>
#include <zephyr/usb/usbd.h>

struct usbd_context *app_usbd_init_device(usbd_msg_cb_t msg_cb); // configure and initalize USB

#endif