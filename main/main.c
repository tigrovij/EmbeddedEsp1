#include "debounce/debounce.h"

void app_main(void)
{
    // The button has an external 10 kOhm pull-up and is active low.
    const gpio_config_t button = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&button));
    button_run();
}
