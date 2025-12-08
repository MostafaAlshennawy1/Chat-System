#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <thread>
#include <mutex>
#include <string>
#include <vector>
std::vector<SOCKET> clients;
std::mutex clientsMutex;
#include <algorithm>

int main() {
    WSADATA wsaData;
    int wsaResult = WSAStartup(MAKEWORD(2 ,2) , &wsaData);
    if (wsaResult !=0 ) {
        std::cerr << "WSAStartup failed with error" << wsaResult << std::endl;
        return 1;
    }
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "socket() failed with error " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port= htons(54000);
    if (bind(listenSocket , (sockaddr*)&serverAddr, sizeof(serverAddr))== SOCKET_ERROR) {
        std::cerr << "bind() failed with error"<< WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    if (listen(listenSocket,  SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "listen failed" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Server is Listening now..." << std::endl;
std::vector<std::thread> threads;
    std::mutex countMutex;
    auto handleClient = [&] (SOCKET clientSocket) {
        char buffer[1024];
        int bytesRecived;
        while ((bytesRecived = recv(clientSocket , buffer , sizeof(buffer) , 0)) > 0) {
            if (bytesRecived == SOCKET_ERROR) {
                std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
                break;
            }
            if (bytesRecived == 0) break;

        buffer[bytesRecived] = '\0';
            {
                std::lock_guard<std::mutex> clientsLock(clientsMutex);
                for (SOCKET s : clients) {
                    if (s != clientSocket)
                        send(s, buffer, bytesRecived, 0);
                }
            }
        }
        std::lock_guard<std::mutex> clientslock(clientsMutex);
        clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());
        closesocket(clientSocket);
    };
    while (true) {
        sockaddr_in clientAddress;
        int clientsize = sizeof(clientAddress);
        SOCKET clientsocket = accept(listenSocket , (sockaddr*)&clientAddress , &clientsize);
        if (clientsocket == INVALID_SOCKET) {
            std::cerr << "Accept failed" << WSAGetLastError() << std::endl;
            continue;
        }
        std::cout << "+1 Client Connected: " << inet_ntoa(clientAddress.sin_addr) << std::endl;
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(clientsocket);
        }
        threads.emplace_back(handleClient, clientsocket);
    }

    for (auto& t : threads) t.join ();
    closesocket(listenSocket);
    WSACleanup();

    }