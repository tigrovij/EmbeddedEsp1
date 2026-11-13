idf_component_register(SRCS "main.c" "exhaust_fan.c"
                       PRIV_REQUIRES esp_driver_gpio esp_timer log
                       INCLUDE_DIRS "")
