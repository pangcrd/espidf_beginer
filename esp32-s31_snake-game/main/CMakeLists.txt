idf_component_register(SRCS ${app_sources}
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES joystick led_strip
)
