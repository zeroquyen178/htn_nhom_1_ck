#ifndef BUTTON_H
#define BUTTON_H

#include "driver/gpio.h"

typedef struct {
    gpio_num_t gpio_pin;
    int counter_id;
} button_config_t;

void button_init(button_config_t *config);

#endif