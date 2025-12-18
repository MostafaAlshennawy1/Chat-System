cmake_minimum_required(VERSION 3.10)
project(ChatClient2)

set(CMAKE_CXX_STANDARD 17)

add_executable(ChatClient2 main.cpp clienttt.cpp)

# ربط مكتبة WinSock على ويندوز
target_link_libraries(ChatClient2 ws2_32)