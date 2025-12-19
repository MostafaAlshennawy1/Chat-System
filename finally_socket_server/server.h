#pragma once
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <vector>
#include <mutex>
#include <string>
#include <atomic>

extern std::vector<SOCKET> clients;
extern std::mutex clientsMutex;

extern std::atomic<bool> serverRunning;
extern std::atomic<bool> shouldStopServer;

extern std::vector<std::string> logMessages;
extern std::mutex logMutex;

void addLog(const std::string& msg);
void startServerThread(unsigned short port);
void stopServer();
void serverThreadFunc(unsigned short port);
void handleClientFn(SOCKET clientSocket);