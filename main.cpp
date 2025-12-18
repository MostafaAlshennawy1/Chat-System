#include <iostream>
#include <string>
#include "clienttt.h"

int main() {
    std::cout << "=== Simple Chat Client ===" << std::endl;
    std::cout << "==========================" << std::endl;

    std::string serverIP;
    unsigned short port = 54000;

    std::cout << "Enter server IP address: ";
    std::getline(std::cin, serverIP);

    std::cout << "Enter server port: ";
    std::cin >> port;
    std::cin.ignore();

    startClient(serverIP, port);

    return 0;
}
