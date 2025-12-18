#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <string>
#include <atomic>
#include <chrono>
#include "clienttt.h"

#pragma comment(lib, "ws2_32.lib")

std::atomic<bool> g_running(true);

void receiveMessages(SOCKET clientSocket) {
    char buffer[4096];
    int bytesReceived;


    u_long mode = 1;
    ioctlsocket(clientSocket, FIONBIO, &mode);

    while (g_running.load()) {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::cout << "\r" << buffer << "\nYou: " << std::flush;
        }
        else if (bytesReceived == 0) {
            std::cout << "\r[Server] Connection closed by server.\n";
            g_running.store(false);
            break;
        }
        else {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            else {
                std::cerr << "\r[Error] Receive failed: " << error << "\n";
                g_running.store(false);
                break;
            }
        }
    }
}

void sendMessages(SOCKET clientSocket, const std::string& username) {
    std::string message;

    while (g_running.load()) {
        std::cout << "You: ";
        if (!std::getline(std::cin, message)) {

            g_running.store(false);
            break;
        }


        if (message.empty()) {
            continue;
        }

        if (message == "/exit" || message == "/quit") {
            g_running.store(false);
            break;
        }


        std::string fullMessage;
        if (message.substr(0, 3) == "/w ") {

            fullMessage = "[PM from " + username + "] " + message.substr(3);
        }
        else if (message.substr(0, 6) == "/clear") {

            system("cls");
            std::cout << "Chat cleared.\n";
            continue;
        }
        else {
            fullMessage = "[" + username + "] " + message;
        }

        int sendResult = send(clientSocket, fullMessage.c_str(), fullMessage.length(), 0);
        if (sendResult == SOCKET_ERROR) {
            int error = WSAGetLastError();
            if (g_running.load()) {
                std::cerr << "\r[Error] Send failed: " << error << "\n";
                g_running.store(false);
            }
            break;
        }
    }
}

void startClient(const std::string& serverIP, unsigned short port) {

    std::string username;
    while (username.empty()) {
        std::cout << "Enter your username: ";
        std::getline(std::cin, username);

        if (username.empty()) {
            std::cout << "Username cannot be empty!\n";
        }
        else if (username.length() > 20) {
            std::cout << "Username too long (max 20 characters)!\n";
            username.clear();
        }
    }


    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return;
    }


    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }


    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);


    if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) != 1) {

        serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
        if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
            std::cerr << "Invalid IP address format: " << serverIP << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            return;
        }
    }


    std::cout << "Connecting to " << serverIP << ":" << port << "...\n";

    result = connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        std::cerr << "Connection failed: " << error << std::endl;

        
        if (error == WSAECONNREFUSED) {
            std::cerr << "Server refused the connection. Make sure the server is running.\n";
        }
        else if (error == WSAETIMEDOUT) {
            std::cerr << "Connection timed out.\n";
        }

        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    std::cout << "\n=== Connected successfully! ===\n";
    std::cout << "Username: " << username << "\n";
    std::cout << "Server: " << serverIP << ":" << port << "\n";
    std::cout << "================================\n";
    std::cout << "Available commands:\n";
    std::cout << "  /exit or /quit - Leave chat\n";
    std::cout << "  /w [user] [msg] - Private message\n";
    std::cout << "  /clear - Clear screen\n";
    std::cout << "================================\n\n";


    std::string joinMessage = "[System] " + username + " has joined the chat!";
    send(clientSocket, joinMessage.c_str(), joinMessage.length(), 0);


    std::thread receiverThread(receiveMessages, clientSocket);
    receiverThread.detach();


    sendMessages(clientSocket, username);


    g_running.store(false);


    std::string leaveMessage = "[System] " + username + " has left the chat.";
    send(clientSocket, leaveMessage.c_str(), leaveMessage.length(), 0);


    shutdown(clientSocket, SD_BOTH);


    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    closesocket(clientSocket);
    WSACleanup();

    std::cout << "\nDisconnected from server. Press Enter to exit...\n";
    std::cin.get();
}