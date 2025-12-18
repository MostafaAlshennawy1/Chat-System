#include <windows.h>
#include "shared.hpp"
#include <string>
#include <vector>

#define IDC_HISTORY 101
#define IDC_INPUT   102
#define IDC_SEND    103
#define IDC_USERNAME 104
#define IDC_JOIN    105
#define TIMER_ID    1

HWND hHistory, hInput, hSend, hUsername, hJoin, hLabel;
std::string my_username;
int my_user_id = -1;
bool joined = false;


void SetFont(HWND hwnd) {
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
}


void AppendText(HWND hEdit, const std::string& text) {
    int len = GetWindowTextLength(hEdit);
    SendMessage(hEdit, EM_SETSEL, len, len);

    std::string fixed_text;
    for (char c : text) {
        if (c == '\n') fixed_text += "\r\n";
        else fixed_text += c;
    }
    fixed_text += "\r\n";

    SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)fixed_text.c_str());
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_CREATE:
            // Login controls
            hLabel = CreateWindow("STATIC", "Enter Username:", WS_CHILD | WS_VISIBLE, 10, 14, 100, 20, hwnd, NULL, NULL, NULL);
            SetFont(hLabel);

            hUsername = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 110, 10, 150, 24, hwnd, (HMENU)IDC_USERNAME, NULL, NULL);
            SetFont(hUsername);

            hJoin = CreateWindow("BUTTON", "Join Chat", WS_CHILD | WS_VISIBLE, 270, 10, 100, 24, hwnd, (HMENU)IDC_JOIN, NULL, NULL);
            SetFont(hJoin);


            hHistory = CreateWindow("EDIT", "", WS_CHILD | WS_BORDER | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL, 10, 50, 365, 200, hwnd, (HMENU)IDC_HISTORY, NULL, NULL);
            SetFont(hHistory);

            hInput = CreateWindow("EDIT", "", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 10, 260, 270, 24, hwnd, (HMENU)IDC_INPUT, NULL, NULL);
            SetFont(hInput);

            hSend = CreateWindow("BUTTON", "Send", WS_CHILD, 290, 260, 85, 24, hwnd, (HMENU)IDC_SEND, NULL, NULL);
            SetFont(hSend);
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_JOIN) {
                char buf[32];
                GetWindowText(hUsername, buf, 32);
                my_username = buf;
                if (my_username.empty()) {
                    MessageBox(hwnd, "Please enter a username", "Error", MB_ICONEXCLAMATION);
                    return 0;
                }


                if (!init_shared_memory(false)) {
                    // Failed to join, try to create (User 1)
                    if (!init_shared_memory(true)) {
                        MessageBox(hwnd, "Failed to initialize shared memory.\nIs another instance running?", "Error", MB_ICONERROR);
                        return 0;
                    }
                }

                SharedBuffer* buffer = get_shared_buffer();
                my_user_id = register_user(buffer);
                if (my_user_id == -1) {
                    MessageBox(hwnd, "Chat is full (Max 2 users).", "Error", MB_ICONERROR);
                    cleanup_shared_memory();
                    return 0;
                }

                joined = true;
                ShowWindow(hLabel, SW_HIDE);
                ShowWindow(hUsername, SW_HIDE);
                ShowWindow(hJoin, SW_HIDE);
                ShowWindow(hHistory, SW_SHOW);
                ShowWindow(hInput, SW_SHOW);
                ShowWindow(hSend, SW_SHOW);

                SetFocus(hInput);
                SetTimer(hwnd, TIMER_ID, 200, NULL); // Check for messages every 200ms

                AppendText(hHistory, "--- Connected as " + my_username + " ---");
            }
            else if (LOWORD(wParam) == IDC_SEND) {
                char buf[256];
                GetWindowText(hInput, buf, 256);
                std::string msg = buf;
                if (!msg.empty()) {
                    send_message(get_shared_buffer(), my_username, msg);
                    SetWindowText(hInput, "");

                    std::string displayMsg = "Me: " + msg;
                    AppendText(hHistory, displayMsg);
                }
                SetFocus(hInput);
            }
            break;

        case WM_TIMER:
            if (joined) {
                std::string msg = receive_all_messages(get_shared_buffer(), my_user_id, my_username);
                if (!msg.empty()) {
                    AppendText(hHistory, msg);
                }
            }
            break;

        case WM_DESTROY:
            if (joined) {
                cleanup_shared_memory();
            }
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {0};
    wc.lpszClassName = "ChatWindow";
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);


    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winW = 400;
    int winH = 340;
    int x = (screenW - winW) / 2;
    int y = (screenH - winH) / 2;

    CreateWindow("ChatWindow", "Chat System", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
                 x, y, winW, winH, NULL, NULL, hInstance, NULL);

    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}