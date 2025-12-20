cmake_minimum_required(VERSION 3.10)
project(ChatClientGUII)

set(CMAKE_CXX_STANDARD 17)


add_executable(chatConsole main.cpp clienttt.cpp)
target_link_libraries(chatConsole ws2_32)


add_executable(chatGUI gui_main.cpp)
target_link_libraries(chatGUI ws2_32)

if(MINGW)
    set_target_properties(chatGUI PROPERTIES
            LINK_FLAGS "-mwindows"
            WIN32_EXECUTABLE TRUE)
endif()

if(MSVC)
    set_target_properties(chatGUI PROPERTIES
            LINK_FLAGS "/SUBSYSTEM:WINDOWS"
            WIN32_EXECUTABLE TRUE)
endif()