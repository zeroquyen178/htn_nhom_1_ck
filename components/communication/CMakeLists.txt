idf_component_register(
    SRCS "communication.c"
    INCLUDE_DIRS "."
    REQUIRES esp_http_client json system_events wifi_connection
)