#include "display_lcd.h"
#include "system_events.h"
#include "pins.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "driver/i2c_master.h"
#include "lcd1602/lcd1602.h"
#include <stdio.h>

static const char *TAG = "DISPLAY";

#define I2C_PORT I2C_NUM_0

static i2c_master_bus_handle_t i2c_bus;
static lcd1602_context *lcd;

static void display_task(void *pv)
{
    display_event_t event;
    char line1[32];
    char line2[32];

    while (1)
    {
        // Đợi sự kiện từ queue_manager gửi đến
        if (xQueueReceive(display_queue, &event, portMAX_DELAY))
        {
            ESP_LOGI(TAG, "Update LCD - Counter: %d, Num: %d, Wait: %d",
                     event.counter_id, event.queue_number, event.waiting);

            lcd1602_clear(lcd);

            /* LOGIC HIỂN THỊ PHÂN BIỆT THEO COUNTER_ID */
            if (event.counter_id == -1)
            {
                // Trường hợp 1: Có người mới quét thẻ RFID
                snprintf(line1, sizeof(line1), "NEW TICKET: %d", event.queue_number);
                snprintf(line2, sizeof(line2), "Total Wait: %d", event.waiting);
            }
            else
            {
                // Trường hợp 2: Một quầy cụ thể đang gọi số
                snprintf(line1, sizeof(line1), "COUNTER %d -> %d", event.counter_id + 1, event.queue_number);
                snprintf(line2, sizeof(line2), "Remaining: %d", event.waiting);
            }

            // Đẩy dữ liệu ra LCD
            lcd1602_set_cursor(lcd, 0, 0);
            lcd1602_string(lcd, line1);

            lcd1602_set_cursor(lcd, 1, 0);
            lcd1602_string(lcd, line2);
        }
    }
}

void display_lcd_init(void)
{
    ESP_LOGI(TAG, "Initializing LCD1602 via I2C...");

    /* Cấu hình I2C Master Bus */
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t err = i2c_new_master_bus(&bus_cfg, &i2c_bus);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize I2C bus: %s", esp_err_to_name(err));
        return;
    }

    /* Khởi tạo Driver LCD1602 */
    i2c_lowlevel_config config = {0};
    config.bus = &i2c_bus;

    lcd = lcd1602_init(LCD1602_I2C_ADDRESS_DEFAULT, true, &config);

    if (lcd == NULL)
    {
        ESP_LOGE(TAG, "LCD initialization failed! Check wiring/address.");
        return;
    }

    /* Màn hình bắt đầu */
    lcd1602_clear(lcd);
    lcd1602_string(lcd, "  SMART QUEUE  ");
    lcd1602_set_cursor(lcd, 1, 0);
    lcd1602_string(lcd, "  SYSTEM READY ");

    /* Tạo Task hiển thị */
    xTaskCreate(
        display_task,
        "display_task",
        3072,
        NULL,
        3,
        NULL);
}