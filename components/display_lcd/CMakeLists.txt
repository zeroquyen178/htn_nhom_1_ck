idf_component_register(
    SRCS "display_lcd.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES
        system_events
        system_config
        esp_driver_i2c
        lcd1602
)