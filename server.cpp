//
// Created by KTS on 11/12/2025.
//
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#include "server.h"

#include <iostream>
#include <thread>
#include <string>
#include <algorithm>
#include <vector>
#include <mutex>

std::vector<SOCKET> clients;
std::mutex clientsMutex;

void handleClientFn(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived;

    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytesReceived] = '\0';

        {
            std::lock_guard<std::mutex> clientsLock(clientsMutex);
            for (SOCKET s : clients) {
                if (s != clientSocket) {
                    send(s, buffer, bytesReceived, 0);
                }
            }
        }
    }

    {
        std::lock_guard<std::mutex> clientsLock(clientsMutex);
        clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());
    }
    closesocket(clientSocket);
}

void startServer(unsigned short port) {
    WSADATA wsaData;
    int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaResult != 0) {
        std::cerr << "WSAStartup failed with error " << wsaResult << std::endl;
        return;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "socket() failed with error " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "bind() failed with error " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "listen failed: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    std::cout << "Server is Listening now on port " << port << "..." << std::endl;

    std::vector<std::thread> threads;

    while (true) {
        sockaddr_in clientAddress{};
        int clientSize = sizeof(clientAddress);
        SOCKET clientSocket = accept(listenSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientSize);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        std::cout << "+1 Client Connected: " << inet_ntoa(clientAddress.sin_addr) << std::endl;

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(clientSocket);
        }

        threads.emplace_back(handleClientFn, clientSocket);
    }

    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    closesocket(listenSocket);
    WSACleanup();
}