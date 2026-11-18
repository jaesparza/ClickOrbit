idf_component_register(
    SRCS "battery.c" "pomodoro.c" "pomodoro_store.c" "pomodoro_view.c" "app.c" "config_view.c" "app_screen.c" "dial_style.c" "lock_combination.c" "app_upload.c" "app_serve.c"
    INCLUDE_DIRS "include"
    REQUIRES ui theme haptic net keyboard esp_timer
    PRIV_REQUIRES nvs_flash esp_adc
)
