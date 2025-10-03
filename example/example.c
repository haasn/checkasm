#include "example.h"

/* Example CPU flags (dummy) */
static uint64_t cpu_flag_mask = (uint64_t) -1;
uint64_t example_get_cpu_flags(void)
{
    return cpu_flag_mask;
}

void example_set_cpu_flags(uint64_t flags)
{
    cpu_flag_mask = flags;
}

/* Example function */
void *nihcpy(void *dest, const void *src, size_t n)
{
    char *cdest = (char *) dest;
    const char *csrc = (const char *) src;
    while (n-- > 0)
        *cdest++ = *csrc++;
    return dest;
}

nihcpy_func get_nihcpy_func(void)
{
    const unsigned flags = example_get_cpu_flags();
    if (flags & EXAMPLE_CPU_FLAG_X86)
        return nihcpy_x86;
    else
        return nihcpy;
}
