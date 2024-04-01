cmake_minimum_required(VERSION 3.27)
project(chip8_emulator C)

set(CMAKE_C_STANDARD 17)

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(SDL2_PATH "SDL2-2.30.1")
set(CMAKE_PREFIX_PATH ${SDL2_PATH})



find_package(SDL2 REQUIRED)
include_directories(SDL2Test ${SDL2_INCLUDE_DIRS})
add_executable(chip8_emulator main.c
        stack.c
        stack.h)

target_link_libraries(chip8_emulator ${SDL2_LIBRARIES})