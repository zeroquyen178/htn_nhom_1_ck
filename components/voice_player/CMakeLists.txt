idf_component_register(
    SRCS "voice_player.c"
    INCLUDE_DIRS "."
    REQUIRES driver log freertos system_events system_config
)