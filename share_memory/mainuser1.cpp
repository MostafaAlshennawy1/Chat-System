#include "shared.hpp"
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

using namespace std;
atomic<bool> running(true);
string my_username;
int my_user_id = -1;

void receiver_thread(SharedBuffer* buffer) {
    while (running) {
        string msg = receive_all_messages(buffer, my_user_id, my_username);
        if (!msg.empty()) {
            cout << "\r" << msg << "\n> ";
            cout.flush();
        }
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

int main() {
    // Change to 'false' for user2
    if (!init_shared_memory(true)) {
        cerr << "Initialization failed. (Make sure User 1 starts first for User 2 to join)\n";
        return 1;
    }

    SharedBuffer* buffer = get_shared_buffer();
    cout << "Enter username: ";
    getline(cin, my_username);
    my_user_id = register_user(buffer);

    if (my_user_id == -1) {
        cerr << "Chat full.\n";
        return 1;
    }

    thread receiver(receiver_thread, buffer);

    cout << "Type messages below (type 'exit' to quit):\n";
    while (running) {
        cout << "> ";
        string input;
        getline(cin, input);
        if (input == "exit") {
            running = false;
            break;
        }
        if (!input.empty()) {
            send_message(buffer, my_username, input);
        }
    }

    if (receiver.joinable()) receiver.join();
    cleanup_shared_memory();
    return 0;
}