# Font generation

set(Pix32_Font_PATH ${CMAKE_CURRENT_LIST_DIR}/Pix32_Font.png)
set(On_Icon_PATH ${CMAKE_CURRENT_LIST_DIR}/On_Icon.png)
set(Chan_Icons_PATH ${CMAKE_CURRENT_LIST_DIR}/Chan_Icons.png)

set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen_fonts)
set(Pix32_Font_H_PATH ${GEN_DIR}/Pix32_Font.h)
set(Chan_Icons_H_PATH ${GEN_DIR}/Chan_Icons.h)
set(On_Icon_H_PATH ${GEN_DIR}/On_Icon.h)

add_library(fonts INTERFACE)
target_include_directories(fonts INTERFACE ${GEN_DIR})

find_package(Python3)

IF (CMAKE_HOST_WIN32)
set(VENV_BIN_DIR Scripts)
ELSE()
set(VENV_BIN_DIR bin)
ENDIF()

add_custom_command(
    OUTPUT font_venv/ready # This must be a file
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/requirements.txt
    COMMAND ${Python3_EXECUTABLE} -m venv font_venv
    COMMAND ./font_venv/${VENV_BIN_DIR}/pip install -r "${CMAKE_CURRENT_LIST_DIR}/requirements.txt"
    COMMAND echo "ready" > ./font_venv/ready # signify the install was a success
)

add_custom_command(
        OUTPUT ${Pix32_Font_H_PATH}
        DEPENDS ${Pix32_Font_PATH} font_venv/ready
        COMMAND ./font_venv/${VENV_BIN_DIR}/python "${CMAKE_CURRENT_LIST_DIR}/font_to_header.py"
                -W 6 -H 12
                "${Pix32_Font_PATH}"
                -o "${Pix32_Font_H_PATH}"
                Pix32_Font
)
add_custom_target(Pix32_Font_H DEPENDS ${Pix32_Font_H_PATH})
add_dependencies(fonts Pix32_Font_H)

add_custom_command(
    OUTPUT ${On_Icon_H_PATH}
    DEPENDS ${On_Icon_PATH} font_venv/ready
    COMMAND ./font_venv/${VENV_BIN_DIR}/python "${CMAKE_CURRENT_LIST_DIR}/font_to_header.py"
            -W 13 -H 11 -s 65 -c 1
            "${On_Icon_PATH}"
            -o "${On_Icon_H_PATH}"
            On_Icon
)
add_custom_target(On_Icon_H DEPENDS ${On_Icon_H_PATH})
add_dependencies(fonts On_Icon_H)

add_custom_command(
    OUTPUT ${Chan_Icons_H_PATH}
    DEPENDS ${Chan_Icons_PATH} font_venv/ready
    COMMAND ./font_venv/${VENV_BIN_DIR}/python "${CMAKE_CURRENT_LIST_DIR}/font_to_header.py"
            -W 32 -H 16 -s 65 -c 5
            "${Chan_Icons_PATH}"
            -o "${Chan_Icons_H_PATH}"
            Chan_Icons
)
add_custom_target(Chan_Icons_H DEPENDS ${Chan_Icons_H_PATH})
add_dependencies(fonts Chan_Icons_H)
