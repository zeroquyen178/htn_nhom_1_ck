#include "button.h"
#include "pins.h"
#include "system_events.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "BUTTON";

static void IRAM_ATTR button_isr_handler(void *arg)
{
    button_config_t *config = (button_config_t *)arg;
    service_event_t event;
    event.counter_id = config->counter_id;
    event.action = 1;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Gửi struct sự kiện vào hàng đợi
    xQueueSendFromISR(service_queue, &event, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

void button_init(button_config_t *config)
{
    if (config == NULL)
        return;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << config->gpio_pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE};

    gpio_config(&io_conf);

    gpio_isr_handler_add(config->gpio_pin, button_isr_handler, (void *)config);

    ESP_LOGI(TAG, "Initialized Button on GPIO %d for Counter %d",
             config->gpio_pin, config->counter_id + 1);
}