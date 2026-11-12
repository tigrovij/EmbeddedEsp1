# Select one file: task1_basic.c, task2_time.c, task3_state.c, or task4_polling.c.
set(debounce_source "debounce/task1_basic.c")

idf_component_register(SRCS "main.c" "${debounce_source}"
                       PRIV_REQUIRES esp_driver_gpio esp_timer
                       INCLUDE_DIRS "")
