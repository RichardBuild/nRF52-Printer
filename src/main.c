#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usbd.h>

#include "usb_device.h"
#include "storage.h"

LOG_MODULE_REGISTER(main);

#define USB_HOST_TIMEOUT K_SECONDS(1)

static const struct gpio_dt_spec clk = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), clk_gpios); // get GPIO info from device tree
static const struct gpio_dt_spec sin = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), sin_gpios);
static const struct gpio_dt_spec sout = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), sout_gpios);

static struct gpio_callback clk_cb_data;

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

static void clk_callback(const struct device *dev, struct gpio_callback *cb, gpio_port_pins_t pins)
{
}

static int printer_init(void)
{
        if (!gpio_is_ready_dt(&clk) || !gpio_is_ready_dt(&sin) || !gpio_is_ready_dt(&sout))
        {
                return -ENODEV;
        }

        gpio_pin_configure_dt(&clk, GPIO_INPUT);           // initialize pins
        gpio_pin_configure_dt(&sin, GPIO_OUTPUT_INACTIVE); // sin is output pin since it is input into the gameboy not into the peripheral (set to low)
        gpio_pin_configure_dt(&sout, GPIO_INPUT);

        gpio_init_callback(&clk_cb_data, clk_callback, BIT(clk.pin)); // initialize clk_cb_data
        // BIT(clk.pin) bitmask selecting CLK's pin within the GPIO port.
        gpio_add_callback(clk.port, &clk_cb_data);                 // register callback to gpio controller
        gpio_pin_interrupt_configure_dt(&clk, GPIO_INT_EDGE_BOTH); // specify when interrupt should occur

        return 0;
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