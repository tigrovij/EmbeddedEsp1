#include "nonblocking_blink.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_timer.h"

enum {
    LED1_GPIO = GPIO_NUM_4,
    LED2_GPIO = GPIO_NUM_5,
    LED3_GPIO = GPIO_NUM_6,
    PAUSE_GPIO = GPIO_NUM_7,
    SPEED_GPIO = GPIO_NUM_8,
    LED1_FAST_US = 200000,
    LED1_SLOW_US = 400000,
    SAMPLE_INTERVAL_US = 5000,
    DEBOUNCE_US = 30000,
};

typedef struct {
    gpio_num_t pin;
    int64_t interval_us;
    int64_t next_toggle_us;
    bool on;
} led_t;

typedef struct {
    gpio_num_t pin;
    bool raw_pressed;
    bool stable_pressed;
    int64_t changed_at_us;
} button_t;

static led_t leds[] = {
    { .pin = LED1_GPIO, .interval_us = LED1_FAST_US },
    { .pin = LED2_GPIO, .interval_us = 500000 },
    { .pin = LED3_GPIO, .interval_us = 1000000 },
};

static button_t pause_button = { .pin = PAUSE_GPIO };
static button_t speed_button = { .pin = SPEED_GPIO };
static bool paused;
static int64_t pause_started_us;
static int64_t total_paused_us;

static bool button_was_pressed(button_t *button, int64_t now_us)
{
    const bool pressed = gpio_get_level(button->pin) == 0;
    if (pressed != button->raw_pressed) {
        button->raw_pressed = pressed;
        button->changed_at_us = now_us;
        return false;
    }

    if (pressed == button->stable_pressed || now_us - button->changed_at_us < DEBOUNCE_US) {
        return false;
    }

    button->stable_pressed = pressed;
    return pressed;
}

static void blink_step(void *arg)
{
    (void)arg;
    const int64_t now_us = esp_timer_get_time();

    if (button_was_pressed(&pause_button, now_us)) {
        if (paused) {
            total_paused_us += now_us - pause_started_us;
        } else {
            pause_started_us = now_us;
        }
        paused = !paused;
    }

    const int64_t blink_now_us = (paused ? pause_started_us : now_us) - total_paused_us;

    if (button_was_pressed(&speed_button, now_us)) {
        led_t *led1 = &leds[0];
        led1->interval_us = led1->interval_us == LED1_FAST_US ? LED1_SLOW_US : LED1_FAST_US;
        led1->next_toggle_us = blink_now_us + led1->interval_us;
    }

    if (paused) {
        return;
    }

    for (size_t i = 0; i < sizeof(leds) / sizeof(leds[0]); ++i) {
        led_t *led = &leds[i];
        if (blink_now_us < led->next_toggle_us) {
            continue;
        }

        led->on = !led->on;
        gpio_set_level(led->pin, led->on);
        led->next_toggle_us += led->interval_us;
        if (led->next_toggle_us <= blink_now_us) {
            led->next_toggle_us = blink_now_us + led->interval_us;
        }
    }
}

void nonblocking_blink_start(void)
{
    const gpio_config_t outputs = {
        .pin_bit_mask = (1ULL << LED1_GPIO) | (1ULL << LED2_GPIO) | (1ULL << LED3_GPIO),
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&outputs));

    const gpio_config_t inputs = {
        .pin_bit_mask = (1ULL << PAUSE_GPIO) | (1ULL << SPEED_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&inputs));

    const int64_t now_us = esp_timer_get_time();
    for (size_t i = 0; i < sizeof(leds) / sizeof(leds[0]); ++i) {
        ESP_ERROR_CHECK(gpio_set_level(leds[i].pin, 0));
        leds[i].next_toggle_us = now_us + leds[i].interval_us;
    }

    const esp_timer_create_args_t timer_args = {
        .callback = blink_step,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "blink_step",
        .skip_unhandled_events = true,
    };
    esp_timer_handle_t timer;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer, SAMPLE_INTERVAL_US));
}
