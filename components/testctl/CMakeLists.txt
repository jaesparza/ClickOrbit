idf_component_register(
    SRCS "testctl.c"
    INCLUDE_DIRS "include"
    REQUIRES app ui keyboard esp_tinyusb mbedtls
)
