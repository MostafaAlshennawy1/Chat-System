#ifndef SHARED_HPP
#define SHARED_HPP

#include <string>
#include <windows.h>

#define MAX_MESSAGES 100
#define MESSAGE_LEN 256
#define USERNAME_LEN 32
#define MAX_USERS 2

struct Message {
    char username[USERNAME_LEN];
    char text[MESSAGE_LEN];
    int sequence;
};

struct SharedBuffer {
    int write_index;
    int read_index[MAX_USERS];
    bool system_active;
    int active_users;
    Message messages[MAX_MESSAGES];
};

bool init_shared_memory(bool is_creator);
void cleanup_shared_memory();
bool send_message(SharedBuffer* buffer, const std::string& username, const std::string& text);
std::string receive_all_messages(SharedBuffer* buffer, int user_id, const std::string& my_username);
SharedBuffer* get_shared_buffer();
int register_user(SharedBuffer* buffer);

#endif