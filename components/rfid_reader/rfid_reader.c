#include "rfid_reader.h"
#include "system_events.h"

#include "esp_log.h"
#include "pins.h"
#include "rc522.h"
#include "driver/rc522_spi.h"
#include "rc522_picc.h"

static const char *TAG = "RFID";

/* Cấu hình driver SPI */
static rc522_spi_config_t driver_config = {
    .host_id = SPI2_HOST,

    .bus_config = &(spi_bus_config_t){
        .miso_io_num = RFID_MISO,
        .mosi_io_num = RFID_MOSI,
        .sclk_io_num = RFID_SCK,
    },

    .dev_config = {
        .spics_io_num = RFID_SS,
    },

    .rst_io_num = RFID_RST,
};

static rc522_driver_handle_t driver;
static rc522_handle_t scanner;

/* Event handler khi trạng thái thẻ thay đổi */
static void on_picc_state_changed(void *arg,
                                  esp_event_base_t base,
                                  int32_t event_id,
                                  void *data)
{
    rc522_picc_state_changed_event_t *event = (rc522_picc_state_changed_event_t *)data;
    rc522_picc_t *picc = event->picc;

    if (picc->state == RC522_PICC_STATE_ACTIVE)
    {
        ESP_LOGI(TAG, "RFID card detected");

        rc522_picc_print(picc);

        /* chuyển UID thành số */
        int card_id = (picc->uid.value[0] << 24) |
                      (picc->uid.value[1] << 16) |
                      (picc->uid.value[2] << 8) |
                      (picc->uid.value[3]);

        if (rfid_queue != NULL)
        {
            xQueueSend(rfid_queue, &card_id, portMAX_DELAY);
        }
    }
    else if (picc->state == RC522_PICC_STATE_IDLE &&
             event->old_state >= RC522_PICC_STATE_ACTIVE)
    {
        ESP_LOGI(TAG, "Card removed");
    }
}

void rfid_task_init(void)
{
    ESP_LOGI(TAG, "Initializing RC522");

    /* Create SPI driver */
    rc522_spi_create(&driver_config, &driver);

    /* Install driver */
    rc522_driver_install(driver);

    /* Scanner config */
    rc522_config_t scanner_config = {
        .driver = driver,
    };

    /* Create scanner */
    rc522_create(&scanner_config, &scanner);

    /* Register event */
    rc522_register_events(
        scanner,
        RC522_EVENT_PICC_STATE_CHANGED,
        on_picc_state_changed,
        NULL);

    /* Start scanning */
    rc522_start(scanner);

    ESP_LOGI(TAG, "RC522 scanner started");
}