#ifndef PINS_H
#define PINS_H

#include "driver/gpio.h"

#define VOICE_TX_PIN GPIO_NUM_8
#define VOICE_RX_PIN GPIO_NUM_6

#define RFID_MISO GPIO_NUM_2
#define RFID_MOSI GPIO_NUM_3
#define RFID_SCK GPIO_NUM_4
#define RFID_SS GPIO_NUM_7
#define RFID_RST GPIO_NUM_1

#define I2C_SDA GPIO_NUM_10
#define I2C_SCL GPIO_NUM_18

#define BUTTON_COUNTER_1 GPIO_NUM_0
#define BUTTON_COUNTER_2 GPIO_NUM_5

#endif