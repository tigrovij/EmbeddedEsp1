#include "debounce.h"

#include <inttypes.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void falling_isr(void *arg)
{
    BaseType_t task_woken = pdFALSE;
    // Each interrupt increments the task's notification counter.
    vTaskNotifyGiveFromISR((TaskHandle_t)arg, &task_woken);
    if (task_woken) {
        portYIELD_FROM_ISR();
    }
}

void button_run(void)
{
    button_enable_falling_interrupt(falling_isr, xTaskGetCurrentTaskHandle());

    uint32_t count = 0;
    while (true) {
        count += ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ESP_LOGI("basic", "Falling edges: %" PRIu32, count);
    }
}
