#include "shared.hpp"
#include <iostream>
#include <thread>
#include <atomic>
#include <signal.h>
#include <chrono>
using namespace std;
atomic<bool> running(true);
string my_username;
int my_user_id = -1;

void signal_handler(int) {
    running = false;
}
void receiver_thread(SharedBuffer* buffer) {
    while (running) {
        string msg = receive_all_messages(buffer, my_user_id, my_username);

        if (!msg.empty()) {
            cout << "\n" << msg << endl;
            cout << "> ";
            cout.flush();
        }
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}
int main() {
    signal(SIGINT, signal_handler);
    cout << "=== User 1 - Creating Chat ===\n";
    if (!init_shared_memory(true)) {
        cerr << "Failed to create chat system.\n";
        return 1;
    }
    SharedBuffer* buffer = get_shared_buffer();
    if (!buffer) {
        cerr << "Failed to get shared buffer.\n";
        cleanup_shared_memory();
        return 1;
    }
    cout << "Enter your username: ";
    getline(cin, my_username);
    if (my_username.empty()) my_username = "User1";
    my_user_id = register_user(buffer);
    if (my_user_id == -1) {
        cerr << "Failed to register user.\n";
        cleanup_shared_memory();
        return 1;
    }
    cout << "\n✓ Chat system created!\n";
    cout << "✓ You are: " << my_username << "\n";
    cout << "✓ Type 'exit' to quit\n\n";

    thread receiver(receiver_thread, buffer);

    while (running) {
        cout << "> ";
        string msg;
        getline(cin, msg);

        if (msg == "exit" || msg == "quit") {
            running = false;
            break;
        }
        if (!msg.empty()) {
            if (send_message(buffer, my_username, msg)) {
                cout << "> ";
                cout.flush();
            } else {
                cerr << "Error: Cannot send message.\n";
                break;
            }
        }
    }
    running = false;
    buffer->system_active = false;
    if (receiver.joinable())
        receiver.join();
    cleanup_shared_memory();
    cout << "\nChat ended. Goodbye!\n";

    return 0;
}