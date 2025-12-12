#pragma once
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <vector>
#include <mutex>

extern std::vector<SOCKET> clients;
extern std::mutex clientsMutex;

void startServer(unsigned short port);
void handleClientFn(SOCKET clientSocket);