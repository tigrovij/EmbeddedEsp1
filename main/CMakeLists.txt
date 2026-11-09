idf_component_register(SRCS "main.c" "nonblocking_blink.c"
                       PRIV_REQUIRES esp_driver_gpio esp_timer
                       INCLUDE_DIRS "")
