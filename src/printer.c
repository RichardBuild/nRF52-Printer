#include "printer.h"

#include <zephyr/drivers/gpio.h>
#include <stdint.h>
#include <errno.h>

static const struct gpio_dt_spec clk = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), clk_gpios); // get GPIO info from device tree
static const struct gpio_dt_spec sin = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), sin_gpios);
static const struct gpio_dt_spec sout = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), sout_gpios);

static struct gpio_callback clk_cb_data;

static uint8_t process_byte(uint8_t rx) // state machine, (write more here) returns the byte to shift out during the next byte (GB protocol uses one-byte lag)
{
    return 0;
}

static void clk_cb(const struct device *dev, struct gpio_callback *cb, gpio_port_pins_t pins)
{
    static uint8_t rx_byte;
    static uint8_t tx_byte;

    static uint32_t clk_cnt;

    if (gpio_pin_get_dt(&clk) == 0) // falling edge 1->0, need to set SIN so that the GB can read on rising edge
    {
        gpio_pin_set_dt(&sin, tx_byte & BIT(7)); // get the left most bit and set the gpio
        tx_byte <<= 1;                           // set next bit to left most position
    }
    else // rising edge 0->1, read SOUT (data from GB) (GB will read data from SIN too)
    {
        rx_byte = (rx_byte << 1) | gpio_pin_get_dt(&sout); // shift bits left and set the least significant bit to the SOUT reading
        clk_cnt++;
        if (clk_cnt == 8)
        {
            clk_cnt = 0;
            tx_byte = process_byte(rx_byte);
        }
    }
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

    gpio_init_callback(&clk_cb_data, clk_cb, BIT(clk.pin)); // initialize clk_cb_data
    // BIT(clk.pin) bitmask selecting CLK's pin within the GPIO port.
    gpio_add_callback(clk.port, &clk_cb_data);                 // register callback to gpio controller
    gpio_pin_interrupt_configure_dt(&clk, GPIO_INT_EDGE_BOTH); // specify when interrupt should occur

    return 0;
}