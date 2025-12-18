
// server.cpp

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
// #include <windows.h> // مش لازم هنا، سيبيه لو محتاجة حاجات من Windows API في ملفات أخرى

#include <iostream>
#include <thread>
#include <string>
#include <algorithm>
#include <vector>
#include <mutex>

#pragma comment(lib, "Ws2_32.lib") // لو بتبني بـ MSVC. لو CMake، اربطي ws2_32 في CMakeLists

#include "server.h"

std::vector<SOCKET> clients;
std::mutex clientsMutex;

// ===== Helper لضمان إرسال كل البايتات =====
bool send_all(SOCKET s, const char* data, int len) {
    int total = 0;
    while (total < len) {
        int sent = send(s, data + total, len - total, 0);
        if (sent <= 0) {
            std::cerr << "send() failed on socket " << s
                      << " wsa=" << WSAGetLastError() << "\n";
            return false;
        }
        total += sent;
    }
    return true;
}

// ===== Broadcast بدون قفل طويل =====
void broadcastToOthers(SOCKET from, const char* data, int len) {
    // خدي snapshot من العملاء تحت القفل
    std::vector<SOCKET> snapshot;
    {
        std::lock_guard<std::mutex> lk(clientsMutex);
        snapshot = clients;
    }

    // ابعتي لكل عميل غير المرسل
    for (SOCKET s : snapshot) {
        if (s == from) continue;
        send_all(s, data, len);
    }
}

// ===== تعامل مع عميل واحد =====
void handleClientFn(SOCKET clientSocket) {
    char buffer[1024];

    for (;;) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            // اطبعي اللي وصل
            std::cout << "[recv " << bytesReceived << " bytes] ";
            std::cout.write(buffer, bytesReceived);
            std::cout << "\n";

            // Echo لنفس المرسل (مهم لو فيه عميل واحد)
            send_all(clientSocket, buffer, bytesReceived);

            // Broadcast للباقيين إن وجدوا
            broadcastToOthers(clientSocket, buffer, bytesReceived);
        }
        else if (bytesReceived == 0) {
            std::cout << "Client closed connection.\n";
            break;
        }
        else {
            std::cerr << "recv() failed: " << WSAGetLastError() << "\n";
            break;
        }
    }

    // إزالة العميل وإغلاقه
    {
        std::lock_guard<std::mutex> clientsLock(clientsMutex);
        clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());
    }
    shutdown(clientSocket, SD_BOTH);
    closesocket(clientSocket);
}

// ===== بدء السيرفر =====
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

    // إعداد العنوان
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;   // اسمع على كل الواجهات
    serverAddr.sin_port = htons(port);

    // تقليل احتمال 10048
    BOOL opt = TRUE;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

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

    // لوب استقبال العملاء
    for (;;) {
        sockaddr_in clientAddress{};
        int clientSize = sizeof(clientAddress);
        SOCKET clientSocket = accept(listenSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientSize);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        // اطبعي عنوان العميل (inet_ntop أحدث من inet_ntoa)
        char ip[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &clientAddress.sin_addr, ip, sizeof(ip));
        std::cout << "+1 Client Connected: " << ip << std::endl;

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(clientSocket);
        }

        // شغّلي ثريد العميل (detach علشان مايحصلش leak أو انتظار join)
        std::thread(handleClientFn, clientSocket).detach();
    }

    // (لو هتعملي shutdown منظم، اكسر اللوب فوق، وبعدها اقفلي السوكيت ونضّفي)
    closesocket(listenSocket);
    WSACleanup();
}
