#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <string>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")

std::atomic<bool> g_running(true);

void receiveMessages(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived;

    while (g_running) {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::cout << "\r" << buffer << std::endl;
            std::cout << "You: ";
            std::flush(std::cout);
        }
        else if (bytesReceived == 0) {
            std::cout << "\rServer disconnected." << std::endl;
            g_running = false;
            break;
        }
        else {
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                std::cerr << "\rReceive failed: " << error << std::endl;
                g_running = false;
                break;
            }
        }
    }
}

void sendMessages(SOCKET clientSocket, const std::string& username) {
    std::string message;

    while (g_running) {
        std::cout << "You: ";
        std::getline(std::cin, message);

        if (message == "/exit") {
            g_running = false;
            break;
        }

        std::string fullMessage = username + ": " + message;
        int sendResult = send(clientSocket, fullMessage.c_str(), fullMessage.length(), 0);
        if (sendResult == SOCKET_ERROR) {
            std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
            g_running = false;
            break;
        }
    }
}

void startClient(const std::string& serverIP, unsigned short port) {
    std::string username;
    std::cout << "Enter your username: ";
    std::getline(std::cin, username);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());

    if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
        std::cerr << "Invalid IP address: " << serverIP << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    std::cout << "Connecting to server " << serverIP << ":" << port << "..." << std::endl;
    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    std::cout << "Connected to server successfully!" << std::endl;
    std::cout << "Type your messages (type '/exit' to quit)" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // إرسال رسالة دخول
    std::string joinMessage = username + " has joined the chat!";
    send(clientSocket, joinMessage.c_str(), joinMessage.length(), 0);

    // إنشاء thread لاستقبال الرسائل
    std::thread receiverThread(receiveMessages, clientSocket);

    // thread لإرسال الرسائل
    sendMessages(clientSocket, username);

    // التأكد من إيقاف thread الاستقبال
    g_running = false;
    shutdown(clientSocket, SD_BOTH);
    if (receiverThread.joinable()) {
        receiverThread.join();
    }

    std::string leaveMessage = username + " has left the chat.";
    send(clientSocket, leaveMessage.c_str(), leaveMessage.length(), 0);

    closesocket(clientSocket);
    WSACleanup();

    std::cout << "Disconnected from server." << std::endl;
}
