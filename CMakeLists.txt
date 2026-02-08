cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME F407project)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# 查找LVGL所有源文件
file(GLOB_RECURSE LVGL_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/LVGL/LVGL_SRC/lvgl/src/*.c
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    # 如果你在 BSP 目录下有源文件，也在这里添加
     ${CMAKE_CURRENT_LIST_DIR}/BSP/LED/Led.c
    # 添加LCD驱动源文件
     ${CMAKE_CURRENT_LIST_DIR}/BSP/LCD/lcd.c
     ${CMAKE_CURRENT_LIST_DIR}/BSP/LCD/delay.c
    # 添加TOUCH驱动源文件
     ${CMAKE_CURRENT_LIST_DIR}/BSP/TOUCH/touch.c
    # 添加ESP8266驱动源文件
     ${CMAKE_CURRENT_LIST_DIR}/BSP/ESP8266/esp8266.c
    # 添加LVGL源文件
    ${LVGL_SOURCES}
    # 添加LVGL输入设备驱动源文件
    ${CMAKE_CURRENT_LIST_DIR}/LVGL/LVGL_SRC/lvgl/examples/porting/lv_port_indev.c
    # 添加LVGL显示设备驱动源文件
    ${CMAKE_CURRENT_LIST_DIR}/LVGL/LVGL_SRC/lvgl/examples/porting/lv_port_disp.c
 
    # 添加GUI应用源文件
${CMAKE_CURRENT_LIST_DIR}/LVGL/APPS/gui_app/gui_guider.c
${CMAKE_CURRENT_LIST_DIR}/LVGL/APPS/gui_app/widgets_init.c
${CMAKE_CURRENT_LIST_DIR}/LVGL/APPS/gui_app/events_init.c
${CMAKE_CURRENT_LIST_DIR}/LVGL/APPS/gui_app/setup_scr_screen.c
${CMAKE_CURRENT_LIST_DIR}/LVGL/APPS/gui_app/setup_scr_screen_1.c
${CMAKE_CURRENT_LIST_DIR}/LVGL/APPS/gui_app/setup_scr_screen_2.c
${CMAKE_CURRENT_LIST_DIR}/LVGL/APPS/gui_app/setup_scr_screen_3.c
# 在源文件列表中添加custom.c
${CMAKE_CURRENT_LIST_DIR}/LVGL/APPS/gui_app/custom/custom.c
# 在源文件列表中添加字体文件

${CMAKE_CURRENT_LIST_DIR}/LVGL/APPS/gui_app/guider_fonts/lv_font_ali_12.c
${CMAKE_CURRENT_LIST_DIR}/LVGL/APPS/gui_app/guider_fonts/lv_font_ali_16.c
${CMAKE_CURRENT_LIST_DIR}/LVGL/APPS/gui_app/guider_fonts/lv_font_ali_20.c
${CMAKE_CURRENT_LIST_DIR}/LVGL/APPS/gui_app/guider_fonts/lv_font_ali_60.c


    # 添加图像资源文件
   
    ${CMAKE_CURRENT_LIST_DIR}/LVGL/APPS/gui_app/images/_temputrue_alpha_64x50.c
    ${CMAKE_CURRENT_LIST_DIR}/LVGL/APPS/gui_app/images/_shidu_alpha_64x50.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${CMAKE_CURRENT_LIST_DIR}/BSP
    ${CMAKE_CURRENT_LIST_DIR}/BSP/LED
    ${CMAKE_CURRENT_LIST_DIR}/BSP/LCD
    ${CMAKE_CURRENT_LIST_DIR}/BSP/TOUCH
    ${CMAKE_CURRENT_LIST_DIR}/BSP/ESP8266
    # 添加LVGL头文件路径
    ${CMAKE_CURRENT_LIST_DIR}/LVGL/LVGL_SRC/lvgl
    ${CMAKE_CURRENT_LIST_DIR}/LVGL/LVGL_SRC/lvgl/src
    ${CMAKE_CURRENT_LIST_DIR}/LVGL/LVGL_SRC/lvgl/examples/porting
    # 添加LVGL demo头文件路径
    #${CMAKE_CURRENT_LIST_DIR}/LVGL/APPS/demos
    #${CMAKE_CURRENT_LIST_DIR}/LVGL/APPS/demos/widgets
    ${CMAKE_CURRENT_LIST_DIR}/LVGL/APPS/gui_app
    ${CMAKE_CURRENT_LIST_DIR}/LVGL/APPS/gui_app/custom
    # 在包含路径中添加guider_fonts目录
    ${CMAKE_CURRENT_LIST_DIR}/LVGL/APPS/gui_app/guider_fonts
    # 添加图像资源目录
    ${CMAKE_CURRENT_LIST_DIR}/LVGL/APPS/gui_app/images
    
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    # LVGL配置宏
    LV_CONF_INCLUDE_SIMPLE
    LV_LVGL_H_INCLUDE_SIMPLE
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)