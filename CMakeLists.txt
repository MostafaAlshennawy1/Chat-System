
cmake_minimum_required(VERSION 3.26)

project(finally_socket_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows SDK paths (as detected on your machine)
set(WINSDK_DIR "C:/Program Files (x86)/Windows Kits/10")
set(WINSDK_VER "10.0.26100.0")

# Tell CMake/VS which Windows SDK version to target
set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "${WINSDK_VER}")

# Include Windows SDK headers
include_directories(
        "${WINSDK_DIR}/Include/${WINSDK_VER}/um"
        "${WINSDK_DIR}/Include/${WINSDK_VER}/shared"
        "${WINSDK_DIR}/Include/${WINSDK_VER}/ucrt"
)

# Target: use your project name
add_executable(finally_socket_server
        main.cpp
        server.cpp
        server.h
)

# Link WinSock library
target_link_libraries(finally_socket_server PRIVATE ws2_32)

# MSVC options
if (MSVC)
    target_compile_options(finally_socket_server PRIVATE /EHsc /W4)
endif()
