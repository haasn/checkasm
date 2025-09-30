#include <stddef.h>

typedef void *(*nihcpy_func)(void *dest, const void *src, size_t n);
nihcpy_func get_nihcpy_func(void);

void *nihcpy(void *dest, const void *src, size_t n);
void *nihcpy_x86(void *dest, const void *src, size_t n);
