cmake_minimum_required(VERSION 3.12)

# add_subdirectory(pico-ssd1306/example)

set(NAME blink)
set(OUTPUT_NAME blink)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(${NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_executable(${NAME}
        blink.c
        )

# pull in common dependencies
target_link_libraries(${NAME} pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(blink)

# Copy .uf2 to main dir
add_custom_command(
        TARGET ${NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.uf2
                ${CMAKE_CURRENT_BINARY_DIR}/../${OUTPUT_NAME}.uf2)
