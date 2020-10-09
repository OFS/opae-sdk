#ifndef STI_NOSYS_PROT_ST_DBG_IP_ALLOCATOR_H_INCLUDED
#define STI_NOSYS_PROT_ST_DBG_IP_ALLOCATOR_H_INCLUDED

#include <stdlib.h>

typedef struct {
    char *raw_buff;
    size_t span;
    size_t write_offset;
    size_t read_offset;
    size_t space_available;
} CIRCLE_BUFF;

inline void cbuff_init(CIRCLE_BUFF *cbuff, char *raw_buff, size_t raw_buff_sz) {
    cbuff->raw_buff = raw_buff;
    cbuff->span = raw_buff_sz;
    cbuff->write_offset = 0;
    cbuff->read_offset = 0;
    cbuff->space_available = raw_buff_sz;
}

// No safety here. Assumptions are there is enough valid data to prevent underflowing, and 'amt' is > 0.
inline void cbuff_free(CIRCLE_BUFF *cbuff, size_t amt) {
    cbuff->space_available += amt;
    cbuff->read_offset = (cbuff->read_offset + amt) % cbuff->span;
}

// No safety here. Assumptions are there is enough space to prevent overflowing, and 'amt' is > 0.
inline char *cbuff_alloc(CIRCLE_BUFF *cbuff, size_t amt) {
    cbuff->space_available -= amt;
    char *result = cbuff->raw_buff + cbuff->write_offset;
    cbuff->write_offset = (cbuff->write_offset + amt) % cbuff->span;
    return result;
}

#endif //STI_NOSYS_PROT_ST_DBG_IP_ALLOCATOR_H_INCLUDED