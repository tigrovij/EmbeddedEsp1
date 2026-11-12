#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

#define BUTTON_GPIO GPIO_NUM_4

static inline void button_enable_falling_interrupt(gpio_isr_t handler, void *arg)
{
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_set_intr_type(BUTTON_GPIO, GPIO_INTR_NEGEDGE));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BUTTON_GPIO, handler, arg));
}

void button_run(void);
