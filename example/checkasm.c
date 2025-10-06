#include <checkasm/checkasm.h>
#include "example.h"

void checkasm_check_nihcpy(void);

static const CheckasmCpuFlag flags[] = {
    { "x86", "x86", EXAMPLE_CPU_FLAG_X86 }
};

static const CheckasmTest tests[] = {
    { "nihcpy", checkasm_check_nihcpy },
};

int main(int argc, const char *argv[])
{
    CheckasmConfig cfg = {
        #define ARRAY_LEN(x) (sizeof(x) / sizeof((x)[0]))
        .cpu_flags      = flags,
        .nb_cpu_flags   = ARRAY_LEN(flags),
        .tests          = tests,
        .nb_tests       = ARRAY_LEN(tests),
        .get_cpu_flags  = example_get_cpu_flags,
        .set_cpu_flags  = example_set_cpu_flags,
    };

    return checkasm_main(&cfg, argc, argv);
}
