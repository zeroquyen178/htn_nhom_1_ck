#include "rfid_reader.h"
#include "queue_manager.h"
#include "display_lcd.h"
#include "communication.h"
#include "voice_player.h"
#include "button.h"
#include "system_events.h"
#include "wifi_connection.h"
#include "pins.h"

#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing Smart Queue System with Voice Support...");

    system_events_init();

    gpio_install_isr_service(0);

    static button_config_t btn_counter_1 = {.gpio_pin = BUTTON_COUNTER_1, .counter_id = 0};
    static button_config_t btn_counter_2 = {.gpio_pin = BUTTON_COUNTER_2, .counter_id = 1};

    button_init(&btn_counter_1);
    button_init(&btn_counter_2);

    voice_player_init();

    rfid_task_init();
    display_lcd_init();

    wifi_init_sta();
    communication_init();

    queue_manager_init();

    ESP_LOGI(TAG, "System is ready! Handlers assigned to Core 1.");
}