#include "server.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    Socket Server - Console Version    " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    std::cout << "Starting server on port 54000..." << std::endl;
    startServerThread(54000);


    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (serverRunning) {
        std::cout << std::endl;
        std::cout << "Server is running!" << std::endl;
        std::cout << "Clients can connect to: localhost:54000" << std::endl;
        std::cout << std::endl;
        std::cout << "Press Enter to stop the server..." << std::endl;
        std::cout << "========================================" << std::endl;


        std::thread logThread([]() {
            size_t lastSize = 0;
            while (serverRunning) {
                {
                    std::lock_guard<std::mutex> lock(logMutex);
                    if (logMessages.size() > lastSize) {
                        for (size_t i = lastSize; i < logMessages.size(); i++) {
                            std::cout << "[LOG] " << logMessages[i] << std::endl;
                        }
                        lastSize = logMessages.size();
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
        logThread.detach();

        std::cin.get();
    } else {
        std::cerr << "Failed to start server!" << std::endl;
        return 1;
    }

    std::cout << std::endl;
    std::cout << "Stopping server..." << std::endl;
    stopServer();


    int waitCount = 0;
    while (serverRunning && waitCount < 50) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        waitCount++;
    }

    std::cout << "Server stopped. Goodbye!" << std::endl;
    return 0;
}