idf_component_register(SRCS "keyboard.c" "keyboard_text.c" "usb_descriptors.c" "jiggler_schedule.c" "keyboard_ble.c" INCLUDE_DIRS "include" "." REQUIRES esp_tinyusb esp_timer esp_hid bt)
