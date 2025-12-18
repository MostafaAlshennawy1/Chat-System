cmake_minimum_required(VERSION 3.20)
project(ChatSystem)
set(CMAKE_CXX_STANDARD 17)

add_executable(user1 mainuser1.cpp shared.cpp)
add_executable(user2 mainuser2.cpp shared.cpp)

find_package(Threads REQUIRED)
target_link_libraries(user1 PRIVATE Threads::Threads)
target_link_libraries(user2 PRIVATE Threads::Threads)