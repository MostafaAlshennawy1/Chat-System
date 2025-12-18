#include "shared.hpp"
#include <iostream>
#include <windows.h>

static SharedBuffer* shared_buffer = nullptr;
static HANDLE hMapFile = NULL;
static HANDLE hSemWrite = NULL;
static int current_user_id = -1;

bool init_shared_memory(bool is_creator) {
    const char* shm_name = "Local\\ChatSystemMemory";
    const char* sem_write_name = "Local\\ChatSemWrite";

    if (is_creator) {
        hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedBuffer), shm_name);
        hSemWrite = CreateSemaphoreA(NULL, 1, 1, sem_write_name);
    } else {
        hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, shm_name);
        hSemWrite = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, sem_write_name);
    }

    if (!hMapFile || !hSemWrite) return false;

    shared_buffer = (SharedBuffer*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedBuffer));
    if (!shared_buffer) return false;

    if (is_creator) {
        memset(shared_buffer, 0, sizeof(SharedBuffer));
        shared_buffer->system_active = true;
    }
    return true;
}

int register_user(SharedBuffer* buffer) {
    if (!buffer || buffer->active_users >= MAX_USERS) return -1;
    int id = buffer->active_users++;
    return id;
}

bool send_message(SharedBuffer* buffer, const std::string& username, const std::string& text) {
    if (!buffer) return false;

    WaitForSingleObject(hSemWrite, INFINITE);
    int index = buffer->write_index % MAX_MESSAGES;

    strncpy_s(buffer->messages[index].username, username.c_str(), USERNAME_LEN - 1);
    strncpy_s(buffer->messages[index].text, text.c_str(), MESSAGE_LEN - 1);
    buffer->write_index++;

    ReleaseSemaphore(hSemWrite, 1, NULL);
    return true;
}

std::string receive_all_messages(SharedBuffer* buffer, int user_id, const std::string& my_username) {
    if (!buffer || user_id < 0) return "";
    std::string result;

    WaitForSingleObject(hSemWrite, INFINITE);

    while (buffer->read_index[user_id] < buffer->write_index) {
        int index = buffer->read_index[user_id] % MAX_MESSAGES;


        if (strcmp(buffer->messages[index].username, my_username.c_str()) != 0) {
            if (!result.empty()) result += "\n";
            result += std::string(buffer->messages[index].username) + ": " + buffer->messages[index].text;
        }
        buffer->read_index[user_id]++;
    }

    ReleaseSemaphore(hSemWrite, 1, NULL);
    return result;
}

SharedBuffer* get_shared_buffer() { return shared_buffer; }

void cleanup_shared_memory() {
    if (shared_buffer) UnmapViewOfFile(shared_buffer);
    if (hMapFile) CloseHandle(hMapFile);
    if (hSemWrite) CloseHandle(hSemWrite);
}