# Chat System Project

This project implements a chat system using C++ with two different communication mechanisms:
1.  **Socket-based Chat:** A client-server architecture using TCP sockets (Winsock).
2.  **Shared Memory Chat:** A local chat system using shared memory for inter-process communication.

## Prerequisites

*   C++ Compiler (supporting C++17)
*   CMake (version 3.10 or higher)
*   Windows (due to Winsock and Windows API usage)

## Project Structure

*   **Socket Chat:**
    *   `server.cpp` / `server.h`: Server implementation handling multiple clients.
    *   `clienttt.cpp` / `clienttt.h`: Client implementation.
    *   `main_server.cpp`: Entry point for the server.
    *   `main.cpp`: Entry point for the client.
*   **Shared Memory Chat:**
    *   Located in `share_memory/` directory.
    *   `shared.cpp` / `shared.hpp`: Shared memory logic.
    *   `mainuser1.cpp`: Entry point for User 1.
    *   `mainuser2.cpp`: Entry point for User 2.

## Building the Project

1.  Clone the repository:
    ```sh
    git clone <repository_url>
    cd Chat-System
    ```

2.  Create a build directory and run CMake:
    ```sh
    mkdir build
    cd build
    cmake ..
    cmake --build .
    ```

## Running the Application

### 1. Socket-based Chat

**Start the Server:**
Run the server executable first. It listens on port 54000 by default.
```sh
./ChatServer
```

**Start the Client:**
Run the client executable in a separate terminal window.
```sh
./ChatClient
```
You can run multiple instances of the client to simulate multiple users.

### 2. Shared Memory Chat

Navigate to the `share_memory` build output (or build the specific targets if configured separately).

**Start User 1:**
```sh
./user1
```

**Start User 2:**
```sh
./user2
```

Messages sent by User 1 will appear in User 2's window and vice-versa.

## Features

*   **Multi-threaded Server:** Handles multiple clients simultaneously.
*   **Broadcasting:** Messages sent by one client are broadcast to all other connected clients.
*   **Shared Memory IPC:** Demonstrates inter-process communication using Windows shared memory.
