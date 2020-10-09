#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "common.h"

void generate_expected_handle_message(char *buff, size_t buff_size, const char *sock_name, int handle) {
    snprintf(buff, buff_size, "%s HANDLE=%d", sock_name, handle);
}

int parse_handle_id(const char *buff) {
    const char *HANDLE_MSG = "HANDLE=";
    const size_t handle_msg_strlen = 7;
    char *pos = strstr(buff, HANDLE_MSG);
    if (pos != NULL) {
        return atoi(pos + handle_msg_strlen);
    }
    return -1;
}

void zero_mem(void *a, size_t length) {
    typedef size_t big_type;
    size_t big_size = sizeof(big_type);
    size_t leftover_offset = 0;

    if (length > big_size) {
        size_t fast_transfers = length / big_size;
        leftover_offset = fast_transfers * big_size;
        big_type *big_a = (big_type *)a;
        for (size_t i = 0; i < fast_transfers; ++i) {
            big_a[i] = 0;
        }

    }
    size_t leftover = length % big_size;
    for (size_t i = 0; i < leftover; ++i) {
        ((char *)a)[i + leftover_offset] = 0;
    }
}

void fill_mem(void *a, char c, size_t length) {
    typedef size_t big_type;
    size_t big_size = sizeof(big_type);
    size_t leftover_offset = 0;
    char stamp[32];
    for (unsigned char i = 0; i < big_size; ++i) {
        stamp[i] = c;
    }
    big_type big_stamp = *((big_type *)stamp);

    if (length > big_size) {
        size_t fast_transfers = length / big_size;
        leftover_offset = fast_transfers * big_size;
        big_type *big_a = (big_type *)a;
        for (size_t i = 0; i < fast_transfers; ++i) {
            big_a[i] = big_stamp;
        }

    }
    size_t leftover = length % big_size;
    for (size_t i = 0; i < leftover; ++i) {
        ((char *)a)[i + leftover_offset] = c;
    }
}

int get_random_id() {
    srand((unsigned int)clock() * (unsigned int)time(NULL));
    // We never want a 0 ID
    int result = 0;
    while (result == 0)
        result = rand();

    return result;
}
