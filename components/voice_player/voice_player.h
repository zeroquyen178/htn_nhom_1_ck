#ifndef VOICE_PLAYER_H
#define VOICE_PLAYER_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

extern QueueHandle_t voice_queue;

void voice_player_init(void);

void voice_play_request(int ticket_num, int counter_id);

#endif