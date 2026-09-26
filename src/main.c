#include "usb_device.h"
#include "storage.h"
#include "printer.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usbd.h>

LOG_MODULE_REGISTER(main);

#define USB_HOST_TIMEOUT K_SECONDS(1)

K_SEM_DEFINE(usb_host_sem, 0, 1);

static void msg_cb(struct usbd_context *const usbd_ctx, const struct usbd_msg *const msg)
{
        LOG_INF("USBD message: %s", usbd_msg_type_string(msg->type));

        // bus reset can only be from USB host; using this instead of configuration due to configuration being withheld by accessory approval pop up on MacOS.
        if (msg->type == USBD_MSG_RESET)
        {
                k_sem_give(&usb_host_sem); // host detected; unblock main
        }
        if (msg->type == USBD_MSG_CONFIGURATION)
        {
                LOG_INF("\tConfiguration value %d", msg->status);
        }
}

int main(void)
{
        struct usbd_context *usbd;
        int ret;

        setup_disk();

        usbd = usb_device_init(msg_cb);
        if (usbd == NULL)
        {
                LOG_ERR("Failed to initialize USB device");
                return -ENODEV;
        }

        ret = usbd_enable(usbd);
        if (ret)
        {
                LOG_ERR("Failed to enable device support");
                return ret;
        }

        if (k_sem_take(&usb_host_sem, USB_HOST_TIMEOUT) == 0) // blocking not polling
        {
                LOG_INF("USB host found: mass storage mode");
                return 0;
        }

        LOG_INF("No USB host: Printer mode");

        ret = usbd_disable(usbd); // disable USB to save power
        if (ret)
        {
                LOG_ERR("Failed disable USB");
        }

        ret = printer_init();
        if (ret)
        {
                LOG_ERR("Printer init fail");
                return ret;
        }

        return 0;
}