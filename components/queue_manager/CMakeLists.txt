idf_component_register(
    SRCS "queue_manager.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES system_events voice_player
)