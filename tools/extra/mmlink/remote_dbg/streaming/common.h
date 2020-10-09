#ifndef STI_NOSYS_PROT_COMMON_H_INCLUDED
#define STI_NOSYS_PROT_COMMON_H_INCLUDED

// Enumerations
typedef enum {
    OK,
    FAILURE
} RETURN_CODE;

#define MAX_MACRO(a,b) (((a)>(b))?(a):(b))
#define MIN_MACRO(a,b) (((a)<(b))?(a):(b))

#define USE_ARG_MACRO(arg) arg = arg

#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>

void generate_expected_handle_message(char *buff, size_t buff_size, const char *sock_name, int handle);
int parse_handle_id(const char *buff);
void zero_mem(void *a, size_t length);
void fill_mem(void *a, char c, size_t length);
int get_random_id();

#ifdef __cplusplus
}
#endif

#endif //STI_NOSYS_PROT_COMMON_H_INCLUDED
