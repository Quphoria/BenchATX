cmake_minimum_required(VERSION 3.12)

# add_subdirectory(pico-ssd1306/example)

set(NAME benchatx)
set(OUTPUT_NAME BenchATX)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(${NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_executable(${NAME}
        main.c
        display.c
        power_sensor.c
        status_led.c
        pico-ssd1306/ssd1306.c
)

target_include_directories(${NAME}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/pico-ssd1306
)

# pull in common dependencies
target_link_libraries(${NAME} PUBLIC
        pico_stdlib
        pico_unique_id
        hardware_watchdog
        hardware_pio
        hardware_i2c
)

# Font generation
include(custom_font/fonts.cmake)
target_link_libraries(${NAME} PRIVATE fonts)

# enable USB UART
pico_enable_stdio_usb(${NAME} 1)
pico_enable_stdio_uart(${NAME} 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(${NAME})

# Copy .uf2 to main dir
add_custom_command(
        TARGET ${NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.uf2
                ${CMAKE_CURRENT_LIST_DIR}/${OUTPUT_NAME}.uf2)
