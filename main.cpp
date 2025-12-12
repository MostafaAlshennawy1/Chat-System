// client_final.cpp
#include <iostream>
#include <winsock2.h>
#include <thread>
#include <mutex>
#include <string>

#pragma comment(lib, "ws2_32.lib")

std::mutex displayMutex;
bool isConnected = true;

void receiveMessages(SOCKET sock) {
    char buffer[1024];
    while (isConnected) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::lock_guard<std::mutex> lock(displayMutex);
            std::cout << "\r" << std::string(70, ' ') << "\r"; // Clear line
            std::cout << ">>> " << buffer << std::endl;
            std::cout << "You: " << std::flush;
        }
        else if (bytesReceived == 0) {
            std::lock_guard<std::mutex> lock(displayMutex);
            std::cout << "\rServer disconnected." << std::endl;
            isConnected = false;
            break;
        }
        else {
            std::lock_guard<std::mutex> lock(displayMutex);
            std::cout << "\rConnection error." << std::endl;
            isConnected = false;
            break;
        }
    }
}

int main() {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    // Create socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
        WSACleanup();
        return 1;
    }

    // Server configuration
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(54000);

    // Get server IP
    std::string serverIP;
    std::cout << "=========================================" << std::endl;
    std::cout << "Chat Client" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Enter server IP (default: 127.0.0.1): ";
    std::getline(std::cin, serverIP);

    if (serverIP.empty()) {
        serverIP = "127.0.0.1";
    }
    else if (serverIP == "localhost") {
        serverIP = "127.0.0.1";
    }

    // Convert IP address (works on all Windows versions)
    serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
    if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
        std::cerr << "Invalid IP. Using localhost." << std::endl;
        serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    }

    // Connect to server
    std::cout << "Connecting to " << serverIP << ":54000..." << std::endl;

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed!" << std::endl;
        std::cerr << "Make sure:" << std::endl;
        std::cerr << "1. Server is running (run server.exe first)" << std::endl;
        std::cerr << "2. You entered correct IP address" << std::endl;
        std::cerr << "3. Firewall allows connections" << std::endl;
        closesocket(sock);
        WSACleanup();
        std::cout << "Press Enter to exit...";
        std::cin.get();
        return 1;
    }

    std::cout << "Connected successfully!" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Type your messages below." << std::endl;
    std::cout << "Type 'exit' to disconnect." << std::endl;
    std::cout << "=========================================" << std::endl;

    // Start receiver thread
    std::thread receiver(receiveMessages, sock);
    receiver.detach();

    // Send messages
    std::string message;
    while (isConnected) {
        std::cout << "You: ";
        std::getline(std::cin, message);

        if (message == "exit") {
            std::cout << "Disconnecting..." << std::endl;
            break;
        }

        if (!message.empty()) {
            int sent = send(sock, message.c_str(), message.length(), 0);
            if (sent == SOCKET_ERROR) {
                std::cout << "Failed to send message." << std::endl;
                break;
            }
        }
    }

    // Cleanup
    isConnected = false;
    closesocket(sock);
    WSACleanup();

    std::cout << "Disconnected. Press Enter to exit...";
    std::cin.get();

    return 0;
}
