#include "voice_player.h"
#include "pins.h"
#include "system_events.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "VOICE_PLAYER";

#define DF_UART_PORT UART_NUM_1
#define DF_BAUD_RATE 9600
#define DF_BUF_SIZE 1024

static uint16_t df_checksum(uint8_t *data, uint8_t len)
{
    uint16_t sum = 0;
    for (int i = 1; i < len; i++)
    {
        sum += data[i];
    }
    return -sum;
}

/* Gửi khung truyền 10-byte tới DFPlayer */
static void df_send_cmd(uint8_t cmd, uint16_t parameter)
{
    uint8_t data[10] = {0x7E, 0xFF, 0x06, cmd, 0x00, (uint8_t)(parameter >> 8), (uint8_t)(parameter & 0xFF), 0x00, 0x00, 0xEF};
    uint16_t checksum = df_checksum(data, 7);
    data[7] = (uint8_t)(checksum >> 8);
    data[8] = (uint8_t)(checksum & 0xFF);
    uart_write_bytes(DF_UART_PORT, (const char *)data, 10);
}

/**
 * Task xử lý tuần tự các yêu cầu phát âm thanh
 */
static void voice_player_task(void *pv)
{
    voice_event_t event;
    ESP_LOGI(TAG, "Voice Task Started");

    while (1)
    {
        // Đợi có yêu cầu phát âm thanh
        if (xQueueReceive(voice_queue, &event, portMAX_DELAY))
        {
            ESP_LOGI(TAG, "Playing: Ticket %d at Counter %d", event.ticket_number, event.counter_id + 1);

            // 1. Phát "Mời số thứ tự" (Folder 01, File 001)
            df_send_cmd(0x0F, 0x0101);
            vTaskDelay(pdMS_TO_TICKS(3500)); // Chờ đọc xong cụm 1

            // 2. Phát số thứ tự (Folder 02, File = ticket_num)
            uint16_t ticket_param = (2 << 8) | (event.ticket_number & 0xFF);
            df_send_cmd(0x0F, ticket_param);
            vTaskDelay(pdMS_TO_TICKS(1500));

            // 3. Phát "đến quầy số" (Folder 03, File 001)
            df_send_cmd(0x0F, 0x0301);
            vTaskDelay(pdMS_TO_TICKS(3500));

            // 4. Phát số quầy (Folder 04, File = counter_id + 1)
            uint16_t counter_param = (4 << 8) | ((event.counter_id + 1) & 0xFF);
            df_send_cmd(0x0F, counter_param);

            // Chờ một chút trước khi đọc câu tiếp theo
            vTaskDelay(pdMS_TO_TICKS(1200));
        }
    }
}

void voice_player_init(void)
{
    // Cấu hình UART
    const uart_config_t uart_config = {
        .baud_rate = DF_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_driver_install(DF_UART_PORT, DF_BUF_SIZE, 0, 0, NULL, 0);
    uart_param_config(DF_UART_PORT, &uart_config);
    uart_set_pin(DF_UART_PORT, VOICE_TX_PIN, VOICE_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // Tạo Queue cho Voice
    voice_queue = xQueueCreate(10, sizeof(voice_event_t));

    // Khởi động Task
    xTaskCreate(voice_player_task, "voice_task", 3072, NULL, 3, NULL);

    vTaskDelay(pdMS_TO_TICKS(500));
    df_send_cmd(0x06, 30);
}

void voice_play_request(int ticket_num, int counter_id)
{
    voice_event_t ev = {.ticket_number = ticket_num, .counter_id = counter_id};
    xQueueSend(voice_queue, &ev, 0);
}