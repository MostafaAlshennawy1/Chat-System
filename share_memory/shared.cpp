#include "shared.hpp"
#include <iostream>
#include <cstring>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>
static SharedBuffer* shared_buffer = nullptr;
static int shm_fd = -1;
static int current_user_id = -1;
bool init_shared_memory(bool is_creator) {
    const char* shm_name = "/chat_system";

    if (is_creator) {
        shm_unlink(shm_name);
        shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
        if (shm_fd == -1) {
            perror("shm_open (creator)");
            return false;
        }
        if (ftruncate(shm_fd, sizeof(SharedBuffer)) == -1) {
            perror("ftruncate");
            return false;
        }
    } else {
        shm_fd = shm_open(shm_name, O_RDWR, 0666);
        if (shm_fd == -1) {
            perror("shm_open (joiner)");
            return false;
        }
    }
    shared_buffer = (SharedBuffer*)mmap(
        NULL,
        sizeof(SharedBuffer),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        shm_fd,
        0
    );
    if (shared_buffer == MAP_FAILED) {
        perror("mmap");
        return false;
    }
    if (is_creator) {
        sem_init(&shared_buffer->sem_write, 1, 1);
        sem_init(&shared_buffer->sem_read, 1, 0);
        memset(shared_buffer->messages, 0, sizeof(Message) * MAX_MESSAGES);
        shared_buffer->write_index = 0;
        for (int i = 0; i < MAX_USERS; i++) {
            shared_buffer->read_index[i] = 0;
        }
        shared_buffer->system_active = true;
        shared_buffer->active_users = 0;
    }

    return true;
}
int register_user(SharedBuffer* buffer) {
    if (buffer == nullptr || buffer->active_users >= MAX_USERS) {
        return -1;
    }
    current_user_id = buffer->active_users;
    buffer->active_users++;

    return current_user_id;
}
bool send_message(SharedBuffer* buffer, const std::string& username, const std::string& text) {
    if (buffer == nullptr) return false;

    sem_wait(&buffer->sem_write);

    int index = buffer->write_index % MAX_MESSAGES;

    strncpy(buffer->messages[index].username, username.c_str(), USERNAME_LEN - 1);
    buffer->messages[index].username[USERNAME_LEN - 1] = '\0';

    strncpy(buffer->messages[index].text, text.c_str(), MESSAGE_LEN - 1);
    buffer->messages[index].text[MESSAGE_LEN - 1] = '\0';

    buffer->messages[index].sequence = buffer->write_index;
    buffer->write_index++;

    sem_post(&buffer->sem_read);
    return true;
}

std::string receive_message(SharedBuffer* buffer, int user_id) {
    if (buffer == nullptr || user_id < 0 || user_id >= MAX_USERS) {
        return "";
    }
    std::string result;

    if (buffer->read_index[user_id] < buffer->write_index) {
        int index = buffer->read_index[user_id] % MAX_MESSAGES;

        result = std::string(buffer->messages[index].username) + ": " +
                 std::string(buffer->messages[index].text);

        buffer->read_index[user_id]++;
    }

    return result;
}

std::string receive_all_messages(SharedBuffer* buffer, int user_id, const std::string& my_username) {
    if (buffer == nullptr || user_id < 0 || user_id >= MAX_USERS) {
        return "";
    }

    std::string result;
    bool got_message = false;
    int sem_value;
    sem_getvalue(&buffer->sem_read, &sem_value);
    if (sem_value > 0) {
        sem_wait(&buffer->sem_read);

        while (buffer->read_index[user_id] < buffer->write_index) {
            int index = buffer->read_index[user_id] % MAX_MESSAGES;
            if (strcmp(buffer->messages[index].username, my_username.c_str()) != 0) {
                if (!result.empty()) result += "\n";
                result += std::string(buffer->messages[index].username) + ": " +
                          std::string(buffer->messages[index].text);
                got_message = true;
            }

            buffer->read_index[user_id]++;
        }

        if (got_message) {
            sem_post(&buffer->sem_write);
        } else {
            sem_post(&buffer->sem_read);
        }
    }

    return result;
}
SharedBuffer* get_shared_buffer() {
    return shared_buffer;
}
void cleanup_shared_memory() {
    if (shared_buffer != nullptr && shared_buffer != MAP_FAILED) {
        if (current_user_id != -1 && shared_buffer->active_users > 0) {
            shared_buffer->active_users--;
        }
        munmap(shared_buffer, sizeof(SharedBuffer));
        shared_buffer = nullptr;
    }

    if (shm_fd != -1) {
        close(shm_fd);
        shm_fd = -1;
    }
    current_user_id = -1;
}