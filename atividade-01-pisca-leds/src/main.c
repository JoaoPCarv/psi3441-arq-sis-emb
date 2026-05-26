#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#define SLEEP_TIME_MS 1000

// LEDs pelo Device Tree
#define GREEN_LED_NODE DT_ALIAS(led0)
#define BLUE_LED_NODE  DT_ALIAS(led1)
#define RED_LED_NODE   DT_ALIAS(led2)

#if DT_NODE_HAS_STATUS(GREEN_LED_NODE, okay) && \
    DT_NODE_HAS_STATUS(BLUE_LED_NODE, okay) && \
    DT_NODE_HAS_STATUS(RED_LED_NODE, okay)

static const struct gpio_dt_spec green_led =
    GPIO_DT_SPEC_GET(GREEN_LED_NODE, gpios);

static const struct gpio_dt_spec blue_led =
    GPIO_DT_SPEC_GET(BLUE_LED_NODE, gpios);

static const struct gpio_dt_spec red_led =
    GPIO_DT_SPEC_GET(RED_LED_NODE, gpios);

#else
#error "Board LEDs are not defined in Device Tree"
#endif

// Máquina de estados
enum light_state {
    GREEN,
    BLUE,
    RED
};

int main(void)
{
    // Verifica se os dispositivos estão prontos
    if (!gpio_is_ready_dt(&green_led) ||
        !gpio_is_ready_dt(&blue_led) ||
        !gpio_is_ready_dt(&red_led)) {

        printk("Error: GPIO isn't ready\n");
        return 0;
    }

    // Configura LEDs como saída
    gpio_pin_configure_dt(&green_led, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&blue_led, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&red_led, GPIO_OUTPUT_ACTIVE);

    enum light_state state = GREEN;

    while (1) {

        // Apaga todos os LEDs
        gpio_pin_set_dt(&green_led, 1);
        gpio_pin_set_dt(&blue_led, 1);
        gpio_pin_set_dt(&red_led, 1);

        switch (state) {

            case GREEN:
                gpio_pin_set_dt(&green_led, 0);
                state = BLUE;
                break;

            case BLUE:
                gpio_pin_set_dt(&blue_led, 0);
                state = RED;
                break;

            case RED:
                gpio_pin_set_dt(&red_led, 0);
                state = GREEN;
                break;
        }

        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}