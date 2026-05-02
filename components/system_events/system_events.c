#include "system_events.h"
QueueHandle_t rfid_queue = NULL;
QueueHandle_t display_queue = NULL;
QueueHandle_t service_queue = NULL;
QueueHandle_t motion_queue = NULL;
QueueHandle_t comm_queue = NULL;
QueueHandle_t voice_queue = NULL;

void system_events_init(void)
{
    rfid_queue = xQueueCreate(5, sizeof(int));
    display_queue = xQueueCreate(10, sizeof(display_event_t));
    service_queue = xQueueCreate(10, sizeof(service_event_t));
    motion_queue = xQueueCreate(5, sizeof(int));
    comm_queue = xQueueCreate(10, sizeof(display_event_t));
    voice_queue = xQueueCreate(10, sizeof(voice_event_t));
}