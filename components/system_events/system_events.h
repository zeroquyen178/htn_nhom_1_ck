#ifndef SYSTEM_EVENTS_H
#define SYSTEM_EVENTS_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

extern QueueHandle_t rfid_queue;
extern QueueHandle_t display_queue;
extern QueueHandle_t service_queue;
extern QueueHandle_t motion_queue;
extern QueueHandle_t comm_queue;
extern QueueHandle_t voice_queue;

typedef struct
{
    int counter_id; // ID của quầy (0, 1, 2...)
    int action;     // 1: Call Next, 2: Reset, 3: Recall
} service_event_t;

typedef struct
{
    int counter_id;   // ID quầy (-1: Cấp số mới, >=0: Quầy đang gọi)
    int queue_number; // Số thứ tự
    int waiting;      // Tổng số người đang chờ
} display_event_t;

typedef struct
{
    int ticket_number;
    int counter_id;
} voice_event_t;

void system_events_init(void);

#endif