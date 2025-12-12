cmake_minimum_required(VERSION 3.26)
project(socket_server_clean)

set(CMAKE_CXX_STANDARD 17)

add_executable(socket_server_clean
        main.cpp
        server.cpp
)

target_link_libraries(socket_server_clean ws2_32)
