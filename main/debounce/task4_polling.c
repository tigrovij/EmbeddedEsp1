#include "debounce.h"

#include <inttypes.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

enum {
    SAMPLE_MS = 10,
    STABLE_US = 20000,
};

void button_run(void)
{
    bool stable_pressed = gpio_get_level(BUTTON_GPIO) == 0;
    bool candidate_pressed = stable_pressed;
    int64_t changed_at_us = esp_timer_get_time();
    uint32_t count = 0;

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_MS));
        const bool pressed = gpio_get_level(BUTTON_GPIO) == 0;
        const int64_t now_us = esp_timer_get_time();

        if (pressed != candidate_pressed) {
            candidate_pressed = pressed;
            changed_at_us = now_us;
        } else if (pressed != stable_pressed && now_us - changed_at_us >= STABLE_US) {
            stable_pressed = pressed;
            if (pressed) {
                ESP_LOGI("polling", "Presses: %" PRIu32, ++count);
            }
        }
    }
}
