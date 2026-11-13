#include "exhaust_fan.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#define FAN_GPIO GPIO_NUM_4
#define FAN_ACTIVE_LEVEL 1
#define FAN_PERIOD_SECONDS (60ULL * 60)
#define FAN_ON_SECONDS (15ULL * 60)
#define MICROSECONDS_PER_SECOND 1000000ULL

static const char *const TAG = "fan";
static esp_timer_handle_t fan_stop_timer;

static void fan_stop(void *arg)
{
    (void)arg;
    ESP_ERROR_CHECK(gpio_set_level(FAN_GPIO, !FAN_ACTIVE_LEVEL));
    ESP_LOGI(TAG, "Fan off");
}

static void fan_start(void *arg)
{
    (void)arg;
    esp_err_t err = esp_timer_start_once(fan_stop_timer,
                                         FAN_ON_SECONDS * MICROSECONDS_PER_SECOND);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Fan start skipped: %s", esp_err_to_name(err));
        return;
    }

    ESP_ERROR_CHECK(gpio_set_level(FAN_GPIO, FAN_ACTIVE_LEVEL));
    ESP_LOGI(TAG, "Fan on");
}

void exhaust_fan_start(void)
{
    ESP_ERROR_CHECK(gpio_set_level(FAN_GPIO, !FAN_ACTIVE_LEVEL));
    ESP_ERROR_CHECK(gpio_set_direction(FAN_GPIO, GPIO_MODE_OUTPUT));

    const esp_timer_create_args_t stop_args = {
        .callback = fan_stop,
        .name = "fan_stop",
    };
    const esp_timer_create_args_t start_args = {
        .callback = fan_start,
        .name = "fan_start",
        .skip_unhandled_events = true,
    };
    esp_timer_handle_t fan_start_timer;
    ESP_ERROR_CHECK(esp_timer_create(&stop_args, &fan_stop_timer));
    ESP_ERROR_CHECK(esp_timer_create(&start_args, &fan_start_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(fan_start_timer,
                                             FAN_PERIOD_SECONDS * MICROSECONDS_PER_SECOND));
    ESP_LOGI(TAG, "Fan off, waiting for the first cycle");
}
