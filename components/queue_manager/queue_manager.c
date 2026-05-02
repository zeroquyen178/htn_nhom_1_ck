#include "queue_manager.h"
#include "system_events.h"
#include "voice_player.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include <string.h>

static const char *TAG = "QUEUE_MANAGER";

#define MAX_COUNTERS 5
#define RECALL_TIMEOUT_MS 60000 // Tự động nhắc lại sau mỗi 60 giây

/* Cấu trúc quản lý nội bộ */
typedef struct
{
    int current_serving_number;
    bool is_active;
} counter_info_t;

/* Các biến hệ thống */
static counter_info_t counters[MAX_COUNTERS];
static int total_waiting = 0;
static int ticket_issued_count = 0;

/* Tạo con trỏ mutex và timer */
static SemaphoreHandle_t xQueueMutex = NULL;
static TimerHandle_t xRecallTimer = NULL;

static void vRecallTimerCallback(TimerHandle_t xTimer)
{
    if (xSemaphoreTake(xQueueMutex, portMAX_DELAY))
    {
        if (total_waiting > 0)
        {
            ESP_LOGI(TAG, "Recalling due to customers still waiting...");
            for (int i = 0; i < MAX_COUNTERS; i++)
            {
                if (counters[i].current_serving_number > 0)
                {
                    voice_play_request(counters[i].current_serving_number, i);
                }
            }
        }
        else
        {
            ESP_LOGI(TAG, "Timer Expired but Queue is empty. No recall needed.");
        }
        xSemaphoreGive(xQueueMutex);
    }
}

static void queue_manager_task(void *pv)
{
    int rfid_event;
    service_event_t s_event;
    display_event_t d_event;

    ESP_LOGI(TAG, "Queue Manager Task started with Mutex & Timer Support");

    while (1)
    {
        /*  Khách lấy số mới */
        if (xQueueReceive(rfid_queue, &rfid_event, pdMS_TO_TICKS(50)) == pdPASS)
        {

            if (xSemaphoreTake(xQueueMutex, portMAX_DELAY))
            {
                ticket_issued_count++;
                total_waiting++;

                ESP_LOGI(TAG, "[RFID] New Ticket: %d (Waiting: %d)", ticket_issued_count, total_waiting);

                d_event.counter_id = -1;
                d_event.queue_number = ticket_issued_count;
                d_event.waiting = total_waiting;

                xSemaphoreGive(xQueueMutex); // Trả khóa
            }

            xQueueSend(comm_queue, &d_event, 0);
            xQueueSend(display_queue, &d_event, 0);
        }

        /* Quầy gọi số */
        if (xQueueReceive(service_queue, &s_event, pdMS_TO_TICKS(50)) == pdPASS)
        {
            int cid = s_event.counter_id;
            if (cid >= 0 && cid < MAX_COUNTERS)
            {
                if (xSemaphoreTake(xQueueMutex, portMAX_DELAY))
                {
                    if (total_waiting > 0)
                    {
                        total_waiting--;
                        counters[cid].current_serving_number = ticket_issued_count - total_waiting;

                        ESP_LOGI(TAG, "[Counter %d] Serving: %d", cid + 1, counters[cid].current_serving_number);

                        d_event.counter_id = cid;
                        d_event.queue_number = counters[cid].current_serving_number;
                        d_event.waiting = total_waiting;

                        xQueueSend(display_queue, &d_event, 0);
                        xQueueSend(comm_queue, &d_event, 0);

                        // Phát giọng nói
                        voice_play_request(counters[cid].current_serving_number, cid);

                        // Reset Timer
                        xTimerReset(xRecallTimer, 0);
                    }
                    else
                    {
                        ESP_LOGW(TAG, "Queue empty at Counter %d", cid + 1);
                    }
                    xSemaphoreGive(xQueueMutex);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void queue_manager_init(void)
{
    // Khởi tạo Mutex
    xQueueMutex = xSemaphoreCreateMutex();
    if (xQueueMutex == NULL)
    {
        ESP_LOGE(TAG, "Failed to create Mutex!");
        return;
    }

    // Khởi tạo Software Timer
    xRecallTimer = xTimerCreate("RecallTimer",
                                pdMS_TO_TICKS(RECALL_TIMEOUT_MS),
                                pdTRUE,
                                (void *)0,
                                vRecallTimerCallback);

    if (xRecallTimer != NULL)
    {
        xTimerStart(xRecallTimer, 0);
    }

    for (int i = 0; i < MAX_COUNTERS; i++)
    {
        counters[i].current_serving_number = 0;
        counters[i].is_active = true;
    }

    xTaskCreatePinnedToCore(
        queue_manager_task,
        "queue_mgr_task",
        4096,
        NULL,
        5,
        NULL,
        0);
}
