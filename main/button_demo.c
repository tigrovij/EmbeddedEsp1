#include "button_demo.h"

#include "button_fsm.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUTTON_GPIO GPIO_NUM_4
#define LED_GPIO GPIO_NUM_5
#define POLL_MS 10

static const char *const TAG = "button";

static void log_state(button_state_t state, void *ctx)
{
    (void)state;
    const char *name = ctx;
    ESP_LOGI(TAG, "State: %s", name);
}

static void on_button_event(button_event_t event, void *ctx)
{
    bool *led_on = ctx;
    switch (event) {
    case BUTTON_SHORT:
        *led_on = !*led_on;
        ESP_LOGI(TAG, "SHORT: toggle LED");
        break;
    case BUTTON_DOUBLE:
        *led_on = false;
        ESP_LOGI(TAG, "DOUBLE: LED off");
        break;
    case BUTTON_LONG:
        *led_on = true;
        ESP_LOGI(TAG, "LONG: LED on");
        break;
    case BUTTON_STUCK:
        *led_on = false;
        ESP_LOGW(TAG, "STUCK: check input");
        break;
    }
    ESP_ERROR_CHECK(gpio_set_level(LED_GPIO, *led_on));
}

void button_demo_start(void)
{
    const gpio_config_t input = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
    };
    const gpio_config_t output = {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&input));
    ESP_ERROR_CHECK(gpio_set_level(LED_GPIO, 0));
    ESP_ERROR_CHECK(gpio_config(&output));

    bool led_on = false;
    button_fsm_t button;
    button_fsm_init(&button, esp_timer_get_time());
    button_fsm_set_state_callback(&button, BUTTON_WAITING_FOR_SECOND_PRESS,
                                  log_state, "waiting for second press");
    button_fsm_set_state_callback(&button, BUTTON_SECOND_PRESS,
                                  log_state, "second press");

    const TickType_t interval = pdMS_TO_TICKS(POLL_MS);
    TickType_t last_wake = xTaskGetTickCount();
    while (true) {
        const bool pressed = gpio_get_level(BUTTON_GPIO) == 1;
        button_fsm_poll(&button, pressed, esp_timer_get_time(), on_button_event, &led_on);
        xTaskDelayUntil(&last_wake, interval);
    }
}
