#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec clk = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), clk_gpios); // get GPIO info from device tree
static const struct gpio_dt_spec sin = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), sin_gpios);
static const struct gpio_dt_spec sout = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), sout_gpios);

static struct gpio_callback clk_cb_data;

static void clk_callback(const struct device *dev, struct gpio_callback *cb, gpio_port_pins_t pins)
{
}

int printer_init(void)
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