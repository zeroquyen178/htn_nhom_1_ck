#ifndef WIFI_CONNECTION_H
#define WIFI_CONNECTION_H

#include "esp_err.h"
#include <stdbool.h>
void wifi_init_sta(void);

bool wifi_is_connected(void);

#endif