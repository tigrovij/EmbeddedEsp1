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

static void falling_isr(void *arg)
{
    xTaskNotifyFromISR((TaskHandle_t)arg, 0, eNoAction, NULL);
}

void button_run(void)
{
    bool armed = gpio_get_level(BUTTON_GPIO) != 0;
    int64_t press_since_us = -1;
    int64_t release_since_us = -1;
    uint32_t count = 0;
    button_enable_falling_interrupt(falling_isr, xTaskGetCurrentTaskHandle());

    while (true) {
        const bool edge = xTaskNotifyWait(0, 0, NULL, 0) == pdTRUE;
        const bool pressed = gpio_get_level(BUTTON_GPIO) == 0;
        const int64_t now_us = esp_timer_get_time();

        if (armed) {
            if (!pressed) {
                press_since_us = -1;
            } else if (edge) {
                press_since_us = now_us;
            } else if (press_since_us >= 0 && now_us - press_since_us >= STABLE_US) {
                armed = false;
                press_since_us = -1;
                ESP_LOGI("state", "Presses: %" PRIu32, ++count);
            }
        } else if (pressed || edge) {
            release_since_us = -1;
        } else if (release_since_us < 0) {
            release_since_us = now_us;
        } else if (now_us - release_since_us >= STABLE_US) {
            release_since_us = -1;
            armed = true;
        }

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_MS));
    }
}
