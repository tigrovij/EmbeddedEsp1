idf_component_register(SRCS "main.c" "button_fsm.c" "button_demo.c"
                       PRIV_REQUIRES esp_driver_gpio esp_timer log
                       INCLUDE_DIRS "")
