
cmake_minimum_required(VERSION 3.26)
project(finally_socket_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server_console
        main.cpp
        server.cpp
        server.h
        gui_main.h
)
target_link_libraries(server_console PRIVATE ws2_32)

add_executable(server_gui WIN32
        gui_main.cpp
        gui_main.h
        server.cpp
        server.h
)
target_link_libraries(server_gui PRIVATE user32 gdi32 ws2_32)

if (MSVC)
    target_compile_options(server_console PRIVATE /EHsc /W4)
    target_compile_options(server_gui     PRIVATE /EHsc /W4)
endif()
