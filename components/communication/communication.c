#include "communication.h"
#include "system_events.h"
#include "wifi_connection.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "COMMUNICATION";

#define POST_URL "http://192.168.1.101:5000/queue"
#define GET_NEXT_URL "http://192.168.1.101:5000/get_next"


static void send_http_request(display_event_t *event)
{
    esp_http_client_config_t config = {
        .url = POST_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    cJSON *root = cJSON_CreateObject();
    char cid_str[20];

    // Ánh xạ ID sang tên định dạng chuỗi
    if (event->counter_id == -1)
    {
        strcpy(cid_str, "Kiosk_RFID");
    }
    else
    {
        snprintf(cid_str, sizeof(cid_str), "Counter_%d", event->counter_id + 1);
    }

    cJSON_AddStringToObject(root, "counter_id", cid_str);
    cJSON_AddNumberToObject(root, "queue_number", event->queue_number);
    cJSON_AddNumberToObject(root, "waiting", event->waiting);

    char *post_data = cJSON_PrintUnformatted(root);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Update Server success: %s", cid_str);
    }
    else
    {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(post_data);
}

static void parse_and_dispatch_admin_command(const char *json_data)
{
    // 1. Log dữ liệu thô nhận được
    ESP_LOGI(TAG, "Raw JSON to parse: %s", json_data);

    cJSON *root = cJSON_Parse(json_data);
    if (!root)
    {
        ESP_LOGE(TAG, "JSON Parse Error! Data is not a valid JSON.");
        return;
    }

    // 2. Kiểm tra các trường dữ liệu
    cJSON *action = cJSON_GetObjectItem(root, "action");
    cJSON *cid = cJSON_GetObjectItem(root, "counter_id");

    if (action)
    {
        ESP_LOGI(TAG, "Action found: %s", action->valuestring);
    }
    else
    {
        ESP_LOGW(TAG, "Action field NOT found in JSON");
    }

    if (cid)
    {
        ESP_LOGI(TAG, "Counter ID found: %d", cid->valueint);
    }
    else
    {
        ESP_LOGW(TAG, "Counter ID field NOT found in JSON");
    }

    // 3. Kiểm tra điều kiện thực thi
    if (action && strcmp(action->valuestring, "next") == 0 && cid)
    {
        service_event_t s_event = {.counter_id = cid->valueint};

        // 4. Kiểm tra việc gửi vào Queue
        if (xQueueSend(service_queue, &s_event, 0) == pdPASS)
        {
            ESP_LOGI(TAG, "Successfully forwarded NEXT for Counter %d", s_event.counter_id + 1);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to send to service_queue! Queue might be full.");
        }
    }
    else
    {
        ESP_LOGW(TAG, "Command condition not met (Action not 'next' or missing fields)");
    }

    cJSON_Delete(root);
}

static void fetch_admin_command()
{
    char buffer[256] = {0};
    esp_http_client_config_t config = {
        .url = GET_NEXT_URL,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        int status = esp_http_client_get_status_code(client);
        if (status == 200)
        {
            // Ép ESP32 đọc dữ liệu từ Server vào buffer
            int read_len = esp_http_client_read(client, buffer, sizeof(buffer) - 1);
            if (read_len > 0)
            {
                buffer[read_len] = '\0';
                ESP_LOGI(TAG, "Admin Command Received: %s", buffer);
                parse_and_dispatch_admin_command(buffer);
            }
        }
    }
    else
    {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}
static void admin_listener_task(void *pv)
{
    while (1)
    {
        if (wifi_is_connected())
        {
            fetch_admin_command();
        }
        vTaskDelay(pdMS_TO_TICKS(2500));
    }
}

static void communication_task(void *pv)
{
    display_event_t event;
    while (1)
    {

        if (xQueueReceive(comm_queue, &event, portMAX_DELAY))
        {
            if (wifi_is_connected())
            {
                send_http_request(&event);
            }
        }
    }
}

void communication_init(void)
{

    xTaskCreatePinnedToCore(communication_task, "comm_task", 6144, NULL, 3, NULL, 0);

    xTaskCreatePinnedToCore(admin_listener_task, "admin_task", 10240, NULL, 3, NULL, 0);

    ESP_LOGI(TAG, "Communication Module Initialized");
}
