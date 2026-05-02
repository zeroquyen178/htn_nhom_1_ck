idf_component_register(
    SRCS "rfid_reader.c"
    INCLUDE_DIRS "."
    REQUIRES rc522 system_events driver system_config
)