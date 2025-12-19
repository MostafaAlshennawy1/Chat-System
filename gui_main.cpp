#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

// Global variables
HWND g_hServerIP, g_hPort, g_hUsername, g_hConnectBtn, g_hChatBox, g_hMessageBox, g_hSendBtn;
HWND g_hMainWnd;
std::atomic<bool> g_isConnected(false);
std::atomic<bool> g_running(true);
SOCKET g_clientSocket = INVALID_SOCKET;
std::string g_currentUsername;

// Custom message for updating chat box from thread
#define WM_APPEND_TEXT (WM_USER + 1)

// Function declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void ConnectToServer();
void SendMessageToServer();
void AppendToChatBox(const std::string& text);
void ReceiveMessagesThread();

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "SimpleChatClient";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    g_hMainWnd = CreateWindowEx(
        0, CLASS_NAME, "Simple Chat Client",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 600,
        NULL, NULL, hInstance, NULL
    );

    if (g_hMainWnd == NULL) {
        return 0;
    }

    ShowWindow(g_hMainWnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        CreateWindow("STATIC", "Server IP:", WS_VISIBLE | WS_CHILD,
            20, 20, 100, 20, hwnd, NULL, NULL, NULL);
        g_hServerIP = CreateWindow("EDIT", "127.0.0.1", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            120, 20, 150, 25, hwnd, NULL, NULL, NULL);

        CreateWindow("STATIC", "Port:", WS_VISIBLE | WS_CHILD,
            280, 20, 50, 20, hwnd, NULL, NULL, NULL);
        g_hPort = CreateWindow("EDIT", "54000", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            330, 20, 100, 25, hwnd, NULL, NULL, NULL);

        CreateWindow("STATIC", "Username:", WS_VISIBLE | WS_CHILD,
            20, 55, 100, 20, hwnd, NULL, NULL, NULL);
        g_hUsername = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            120, 55, 200, 25, hwnd, NULL, NULL, NULL);

        g_hConnectBtn = CreateWindow("BUTTON", "Connect", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            330, 55, 100, 25, hwnd, (HMENU)1, NULL, NULL);

        g_hChatBox = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            20, 100, 450, 350, hwnd, NULL, NULL, NULL);

        g_hMessageBox = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            20, 470, 350, 25, hwnd, NULL, NULL, NULL);

        g_hSendBtn = CreateWindow("BUTTON", "Send", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            380, 470, 90, 25, hwnd, (HMENU)2, NULL, NULL);

        // السماح بالكتابة والإرسال حتى بدون اتصال
        EnableWindow(g_hMessageBox, TRUE);
        EnableWindow(g_hSendBtn, TRUE);
        break;
    }

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        if (wmId == 1) {
            ConnectToServer();
        }
        else if (wmId == 2) {
            SendMessageToServer();
        }
        break;
    }

    case WM_APPEND_TEXT: {
        std::string* text = (std::string*)lParam;
        if (text) {
            int len = GetWindowTextLength(g_hChatBox);
            SendMessage(g_hChatBox, EM_SETSEL, len, len);
            SendMessage(g_hChatBox, EM_REPLACESEL, FALSE, (LPARAM)(text->c_str()));
            SendMessage(g_hChatBox, EM_REPLACESEL, FALSE, (LPARAM)"\r\n");
            delete text;
        }
        break;
    }

    case WM_DESTROY:
        g_running.store(false);
        if (g_clientSocket != INVALID_SOCKET) {
            closesocket(g_clientSocket);
            WSACleanup();
        }
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void AppendToChatBox(const std::string& text) {
    std::string* pText = new std::string(text);
    PostMessage(g_hMainWnd, WM_APPEND_TEXT, 0, (LPARAM)pText);
}

void ReceiveMessagesThread() {
    char buffer[4096];
    u_long mode = 1;
    ioctlsocket(g_clientSocket, FIONBIO, &mode);

    while (g_running.load() && g_isConnected.load()) {
        int bytesReceived = recv(g_clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            AppendToChatBox(buffer);
        }
        else if (bytesReceived == 0) {
            AppendToChatBox("[Server] Connection closed by server.");
            g_isConnected.store(false);
            break;
        }
        else {
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                AppendToChatBox("[Error] Receive failed: " + std::to_string(error));
                g_isConnected.store(false);
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void ConnectToServer() {
    char serverIP[256], port[256], username[256];
    GetWindowText(g_hServerIP, serverIP, 256);
    GetWindowText(g_hPort, port, 256);
    GetWindowText(g_hUsername, username, 256);

    if (strlen(username) == 0) {
        MessageBox(g_hMainWnd, "Please enter a username!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    if (strlen(username) > 20) {
        MessageBox(g_hMainWnd, "Username too long (max 20 characters)!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    g_currentUsername = username;
    unsigned short portNum = (unsigned short)atoi(port);

    AppendToChatBox("Connecting to " + std::string(serverIP) + ":" + std::string(port) + "...");

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        AppendToChatBox("[Error] WSAStartup failed: " + std::to_string(result));
        return;
    }

    g_clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_clientSocket == INVALID_SOCKET) {
        AppendToChatBox("[Error] Socket creation failed: " + std::to_string(WSAGetLastError()));
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(portNum);

    if (inet_pton(AF_INET, serverIP, &serverAddr.sin_addr) != 1) {
        serverAddr.sin_addr.s_addr = inet_addr(serverIP);
        if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
            AppendToChatBox("[Error] Invalid IP address format");
            closesocket(g_clientSocket);
            WSACleanup();
            return;
        }
    }

    result = connect(g_clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        AppendToChatBox("[Error] Connection failed: " + std::to_string(error));
        closesocket(g_clientSocket);
        WSACleanup();
        return;
    }

    AppendToChatBox("=== Connected successfully! ===");
    AppendToChatBox("Username: " + g_currentUsername);
    AppendToChatBox("Server: " + std::string(serverIP) + ":" + std::string(port));
    AppendToChatBox("================================");

    std::string joinMessage = "[System] " + g_currentUsername + " has joined the chat!";
    send(g_clientSocket, joinMessage.c_str(), joinMessage.length(), 0);

    g_isConnected.store(true);

    // مش محتاجين نعطل الحقول، خليهم شغالين
    // EnableWindow(g_hServerIP, FALSE);
    // EnableWindow(g_hPort, FALSE);
    // EnableWindow(g_hUsername, FALSE);
    // EnableWindow(g_hConnectBtn, FALSE);
    EnableWindow(g_hMessageBox, TRUE);
    EnableWindow(g_hSendBtn, TRUE);

    std::thread recvThread(ReceiveMessagesThread);
    recvThread.detach();
}

void SendMessageToServer() {
    char message[4096];
    GetWindowText(g_hMessageBox, message, 4096);

    if (strlen(message) == 0) {
        return;
    }

    // إذا كان متصل، يبعت للسيرفر
    if (g_isConnected.load() && g_clientSocket != INVALID_SOCKET) {
        std::string fullMessage = "[" + g_currentUsername + "] " + std::string(message);

        int sendResult = send(g_clientSocket, fullMessage.c_str(), fullMessage.length(), 0);
        if (sendResult == SOCKET_ERROR) {
            int error = WSAGetLastError();
            AppendToChatBox("[Error] Send failed: " + std::to_string(error));
            g_isConnected.store(false);
        } else {
            AppendToChatBox("You: " + std::string(message));
        }
    } else {
        // إذا مش متصل، يعرض الرسالة محلياً فقط
        AppendToChatBox("[Local] You: " + std::string(message));
    }

    SetWindowText(g_hMessageBox, "");
}