#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <thread>
#include <string>
#include <algorithm>
#include <vector>
#include <mutex>
#include <sstream>
#include <atomic>

#pragma comment(lib, "Ws2_32.lib")

#include "server.h"

std::vector<SOCKET> clients;
std::mutex clientsMutex;

std::atomic<bool> serverRunning{false};
std::atomic<bool> shouldStopServer{false};

std::vector<std::string> logMessages;
std::mutex logMutex;

SOCKET listenSocket = INVALID_SOCKET;

void addLog(const std::string& msg) {
    std::lock_guard<std::mutex> lock(logMutex);
    logMessages.push_back(msg);
    if (logMessages.size() > 1000) {
        logMessages.erase(logMessages.begin());
    }
}

bool send_all(SOCKET s, const char* data, int len) {
    int total = 0;
    while (total < len) {
        int sent = send(s, data + total, len - total, 0);
        if (sent <= 0) {
            std::stringstream ss;
            ss << "send() failed on socket " << s << " wsa=" << WSAGetLastError();
            addLog(ss.str());
            return false;
        }
        total += sent;
    }
    return true;
}

void broadcastToOthers(SOCKET from, const char* data, int len) {
    std::vector<SOCKET> snapshot;
    {
        std::lock_guard<std::mutex> lk(clientsMutex);
        snapshot = clients;
    }

    for (SOCKET s : snapshot) {
        if (s == from) continue;
        send_all(s, data, len);
    }
}

void handleClientFn(SOCKET clientSocket) {
    char buffer[1024];

    for (;;) {
        if (shouldStopServer) break;

        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            std::stringstream ss;
            ss << "[recv " << bytesReceived << " bytes] ";
            ss.write(buffer, bytesReceived);
            addLog(ss.str());

            send_all(clientSocket, buffer, bytesReceived);
            broadcastToOthers(clientSocket, buffer, bytesReceived);
        }
        else if (bytesReceived == 0) {
            addLog("Client closed connection.");
            break;
        }
        else {
            std::stringstream ss;
            ss << "recv() failed: " << WSAGetLastError();
            addLog(ss.str());
            break;
        }
    }

    {
        std::lock_guard<std::mutex> clientsLock(clientsMutex);
        clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());
    }
    shutdown(clientSocket, SD_BOTH);
    closesocket(clientSocket);
}

void serverThreadFunc(unsigned short port) {
    WSADATA wsaData;
    int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaResult != 0) {
        std::stringstream ss;
        ss << "WSAStartup failed with error " << wsaResult;
        addLog(ss.str());
        serverRunning = false;
        return;
    }

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::stringstream ss;
        ss << "socket() failed with error " << WSAGetLastError();
        addLog(ss.str());
        WSACleanup();
        serverRunning = false;
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    BOOL opt = TRUE;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    u_long mode = 1;
    ioctlsocket(listenSocket, FIONBIO, &mode);

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::stringstream ss;
        ss << "bind() failed with error " << WSAGetLastError();
        addLog(ss.str());
        closesocket(listenSocket);
        WSACleanup();
        serverRunning = false;
        return;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::stringstream ss;
        ss << "listen failed: " << WSAGetLastError();
        addLog(ss.str());
        closesocket(listenSocket);
        WSACleanup();
        serverRunning = false;
        return;
    }

    std::stringstream ss;
    ss << "Server is Listening on port " << port << "...";
    addLog(ss.str());
    serverRunning = true;

    while (!shouldStopServer) {
        sockaddr_in clientAddress{};
        int clientSize = sizeof(clientAddress);
        SOCKET clientSocket = accept(listenSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientSize);

        if (clientSocket == INVALID_SOCKET) {
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            std::stringstream ss2;
            ss2 << "Accept failed: " << err;
            addLog(ss2.str());
            continue;
        }

        char ip[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &clientAddress.sin_addr, ip, sizeof(ip));

        std::stringstream ss3;
        ss3 << "+1 Client Connected: " << ip;
        addLog(ss3.str());

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(clientSocket);
        }

        std::thread(handleClientFn, clientSocket).detach();
    }

    closesocket(listenSocket);


    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (SOCKET s : clients) {
            shutdown(s, SD_BOTH);
            closesocket(s);
        }
        clients.clear();
    }

    WSACleanup();
    serverRunning = false;
    addLog("Server stopped.");
}

void startServerThread(unsigned short port) {
    if (serverRunning) return;
    shouldStopServer = false;
    std::thread(serverThreadFunc, port).detach();
}

void stopServer() {
    if (!serverRunning) return;
    shouldStopServer = true;
    addLog("Stopping server...");
}