cmake_minimum_required(VERSION 4.1)
project(chatCClient)

set(CMAKE_CXX_STANDARD 20)

add_executable(chatCClient main.cpp clienttt.cpp)

# ربط WinSock على MinGW
target_link_libraries(chatCClient PRIVATE ws2_32)
