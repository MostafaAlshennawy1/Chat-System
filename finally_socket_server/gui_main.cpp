#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <thread>
#include <sstream>
#include "server.h"

#pragma comment(lib, "comctl32.lib")

HWND hTextLog, hEditPort, hBtnStart, hBtnStop, hStatusBar, hLabelClients;
HFONT hFontNormal, hFontBold, hFontTitle;
COLORREF COLOR_BG = RGB(240, 243, 245);

void AppendLogW(const wchar_t* text) {
    int len = GetWindowTextLengthW(hTextLog);
    if (len > 50000) {
        SetWindowTextW(hTextLog, L"");
        len = 0;
    }
    SendMessageW(hTextLog, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageW(hTextLog, EM_REPLACESEL, FALSE, (LPARAM)text);
    SendMessageW(hTextLog, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
    SendMessageW(hTextLog, WM_VSCROLL, SB_BOTTOM, 0);
}

void UpdateStatusW(const wchar_t* text) {
    SendMessageW(hStatusBar, SB_SETTEXT, 0, (LPARAM)text);
}

void UpdateClientCount() {
    std::lock_guard<std::mutex> lock(clientsMutex);
    std::wstringstream ss;
    ss << L"Connected Clients: " << clients.size();
    SetWindowTextW(hLabelClients, ss.str().c_str());
}

static std::wstring ToWideAscii(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}

void UIUpdateThread() {
    size_t lastLogSize = 0;
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        {
            std::lock_guard<std::mutex> lock(logMutex);
            if (logMessages.size() > lastLogSize) {
                for (size_t i = lastLogSize; i < logMessages.size(); i++) {
                    // Expect ASCII only. If you later send UTF-8, replace with proper conversion.
                    std::wstring w = ToWideAscii(logMessages[i]);
                    AppendLogW(w.c_str());
                }
                lastLogSize = logMessages.size();
            }
        }

        UpdateClientCount();

        UpdateStatusW(serverRunning ? L"Server Running" : L"Server Stopped");
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            INITCOMMONCONTROLSEX icex{};
            icex.dwSize = sizeof(icex);
            icex.dwICC = ICC_BAR_CLASSES;
            InitCommonControlsEx(&icex);

            hFontTitle = CreateFontW(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

            hFontBold = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

            hFontNormal = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

            HWND hPanel = CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD | SS_BLACKRECT,
                        0, 0, 850, 120, hwnd, NULL, NULL, NULL);

            HWND hTitle = CreateWindowW(L"STATIC", L"Socket Server Control Panel",
                        WS_VISIBLE | WS_CHILD | SS_CENTER,
                        0, 10, 850, 35, hwnd, NULL, NULL, NULL);
            SendMessageW(hTitle, WM_SETFONT, (WPARAM)hFontTitle, TRUE);

            HWND hLabelPort = CreateWindowW(L"STATIC", L"Port:", WS_VISIBLE | WS_CHILD,
                        30, 60, 60, 25, hwnd, NULL, NULL, NULL);
            SendMessageW(hLabelPort, WM_SETFONT, (WPARAM)hFontBold, TRUE);

            hEditPort = CreateWindowW(L"EDIT", L"54000",
                        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_NUMBER,
                        100, 58, 100, 28, hwnd, NULL, NULL, NULL);
            SendMessageW(hEditPort, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

            hBtnStart = CreateWindowW(L"BUTTON", L"Start Server",
                        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        220, 55, 150, 35, hwnd, (HMENU)1, NULL, NULL);
            SendMessageW(hBtnStart, WM_SETFONT, (WPARAM)hFontBold, TRUE);

            hBtnStop = CreateWindowW(L"BUTTON", L"Stop Server",
                        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        390, 55, 150, 35, hwnd, (HMENU)2, NULL, NULL);
            SendMessageW(hBtnStop, WM_SETFONT, (WPARAM)hFontBold, TRUE);
            EnableWindow(hBtnStop, FALSE);

            hLabelClients = CreateWindowW(L"STATIC", L"Connected Clients: 0",
                        WS_VISIBLE | WS_CHILD,
                        560, 63, 250, 25, hwnd, NULL, NULL, NULL);
            SendMessageW(hLabelClients, WM_SETFONT, (WPARAM)hFontBold, TRUE);

            HWND hLabelLog = CreateWindowW(L"STATIC", L"Server Log:",
                        WS_VISIBLE | WS_CHILD,
                        20, 130, 150, 25, hwnd, NULL, NULL, NULL);
            SendMessageW(hLabelLog, WM_SETFONT, (WPARAM)hFontBold, TRUE);

            hTextLog = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                        WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE |
                        ES_AUTOVSCROLL | ES_READONLY,
                        20, 160, 810, 350, hwnd, NULL, NULL, NULL);
            SendMessageW(hTextLog, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

            hStatusBar = CreateWindowExW(0, STATUSCLASSNAMEW, NULL,
                        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                        0, 0, 0, 0, hwnd, NULL, NULL, NULL);
            UpdateStatusW(L"Server Stopped");

            AppendLogW(L"Welcome to Socket Server Control Panel!");
            AppendLogW(L"");
            AppendLogW(L"Instructions:");
            AppendLogW(L"1. Set your desired port (default: 54000)");
            AppendLogW(L"2. Click 'Start Server' to begin");
            AppendLogW(L"3. Clients can connect to: localhost:<port>");
            AppendLogW(L"");
            AppendLogW(L"--------------------------------------------");
            AppendLogW(L"");

            std::thread uiThread(UIUpdateThread);
            uiThread.detach();

            return 0;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            HWND hControl = (HWND)lParam;
            if (hControl != hTextLog && hControl != hEditPort) {
                SetTextColor(hdcStatic, RGB(255, 255, 255));
                SetBkColor(hdcStatic, RGB(52, 73, 94));
                static HBRUSH hBrush = CreateSolidBrush(RGB(52, 73, 94));
                return (LRESULT)hBrush;
            }
            break;
        }

        case WM_COMMAND: {
            if (LOWORD(wParam) == 1) {
                if (!serverRunning) {
                    wchar_t portStr[16];
                    GetWindowTextW(hEditPort, portStr, 16);
                    unsigned short port = (unsigned short)_wtoi(portStr);

                    if (port < 1024 || port > 65535) {
                        MessageBoxW(hwnd, L"Port must be between 1024 and 65535!",
                                    L"Invalid Port", MB_OK | MB_ICONERROR);
                        return 0;
                    }

                    AppendLogW(L"Starting server...");
                    startServerThread(port);

                    std::this_thread::sleep_for(std::chrono::milliseconds(500));

                    if (serverRunning) {
                        EnableWindow(hBtnStart, FALSE);
                        EnableWindow(hBtnStop, TRUE);
                        EnableWindow(hEditPort, FALSE);
                        AppendLogW(L"Server started successfully!");
                        AppendLogW(L"");
                    } else {
                        AppendLogW(L"Failed to start server!");
                        AppendLogW(L"");
                    }
                }
            }
            else if (LOWORD(wParam) == 2) {
                if (serverRunning) {
                    AppendLogW(L"");
                    AppendLogW(L"Stopping server...");
                    stopServer();

                    int waitCount = 0;
                    while (serverRunning && waitCount < 50) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        waitCount++;
                    }

                    EnableWindow(hBtnStart, TRUE);
                    EnableWindow(hBtnStop, FALSE);
                    EnableWindow(hEditPort, TRUE);
                    AppendLogW(L"Server stopped successfully!");
                    AppendLogW(L"");
                    AppendLogW(L"--------------------------------------------");
                    AppendLogW(L"");
                }
            }
            return 0;
        }

        case WM_SIZE: {
            SendMessageW(hStatusBar, WM_SIZE, 0, 0);
            return 0;
        }

        case WM_CLOSE: {
            if (serverRunning) {
                int result = MessageBoxW(hwnd,
                    L"Server is still running. Are you sure you want to exit?",
                    L"Confirm Exit", MB_YESNO | MB_ICONQUESTION);
                if (result == IDNO) {
                    return 0;
                }
                stopServer();
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            DestroyWindow(hwnd);
            return 0;
        }

        case WM_DESTROY:
            if (serverRunning) {
                stopServer();
            }
            DeleteObject(hFontNormal);
            DeleteObject(hFontBold);
            DeleteObject(hFontTitle);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR /*lpCmdLine*/, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"ServerGUIWindow";
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = CreateSolidBrush(COLOR_BG);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"Socket Server - Control Panel",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 870, 600,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        MessageBoxW(NULL, L"Window Creation Failed!", L"Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);



    MSG msg{};
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}