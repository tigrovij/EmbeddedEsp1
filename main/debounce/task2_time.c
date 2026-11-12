#include "debounce.h"

#include <inttypes.h>
#include <stdatomic.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

enum {
    DEBOUNCE_US = 50000,
    EDGE_QUEUE_LENGTH = 128,
};

static atomic_bool queue_overflow;

static void falling_isr(void *arg)
{
    const int64_t edge_us = esp_timer_get_time();
    BaseType_t task_woken = pdFALSE;
    if (xQueueSendFromISR((QueueHandle_t)arg, &edge_us, &task_woken) != pdPASS) {
        atomic_store_explicit(&queue_overflow, true, memory_order_relaxed);
    }
    if (task_woken) {
        portYIELD_FROM_ISR();
    }
}

void button_run(void)
{
    QueueHandle_t edges = xQueueCreate(EDGE_QUEUE_LENGTH, sizeof(int64_t));
    ESP_ERROR_CHECK(edges ? ESP_OK : ESP_ERR_NO_MEM);
    button_enable_falling_interrupt(falling_isr, edges);

    int64_t previous_edge_us = -1;
    uint32_t count = 0;
    while (true) {
        int64_t edge_us;
        if (xQueueReceive(edges, &edge_us, portMAX_DELAY) != pdPASS) {
            continue;
        }
        if (atomic_exchange_explicit(&queue_overflow, false, memory_order_relaxed)) {
            ESP_LOGW("time", "Edge queue overflow, count may be inaccurate");
        }

        const bool accepted = previous_edge_us < 0 || edge_us - previous_edge_us >= DEBOUNCE_US;
        previous_edge_us = edge_us;
        if (accepted) {
            ESP_LOGI("time", "Accepted edges: %" PRIu32, ++count);
        }
    }
}
