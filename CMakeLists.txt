cmake_minimum_required(VERSION 3.12)

project(grounded LANGUAGES C)

add_library(grounded 
    src/file/grounded_file.c
    src/logger/grounded_logger.c
    src/memory/grounded_memory.c
    src/memory/grounded_arena.c
    src/module/grounded_module.c
    src/string/grounded_string.c
    src/threading/grounded_threading.c
    src/window/grounded_window.c
    src/window/grounded_window_extra.c
)

target_include_directories(grounded PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
