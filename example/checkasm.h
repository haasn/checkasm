#include "../checkasm/checkasm.h"

enum {
    EXAMPLE_CPU_FLAG_X86 = 1 << 0,
};

uint64_t example_get_cpu_flags(void);
void example_set_cpu_flags(uint64_t flags);

void checkasm_check_nihcpy(void);
