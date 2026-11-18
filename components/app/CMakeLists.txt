idf_component_register(
    SRCS "app.c" "app_screen.c" "lock_combination.c" "app_upload.c" "app_serve.c"
    INCLUDE_DIRS "include"
    REQUIRES ui theme haptic net keyboard
)
