CPPFLAGS = -I. -Iinclude -Isrc -Itests
CFLAGS = -std=gnu11 -Wundef
LIBS = -lm
EXE = checkasm-selftest
OBJS = \
	src/arm/checkasm_32.o \
	src/arm/checkasm_64.o \
	src/arm/cpu.o \
	src/loongarch/checkasm.o \
	src/riscv/callcheck.o \
	src/riscv/cpu.o \
	src/x86/cpu.o \
	src/perf/arm.o \
	src/perf/linux.o \
	src/perf/macos_kperf.o \
	src/checkasm.o \
	src/cpu.o \
	src/function.o \
	src/perf.o \
	src/signal.o \
	src/stackguard.o \
	src/stats.o \
	src/utils.o \
	tests/selftest.o \
	tests/generic.o \
	tests/arm/32/tests.o \
	tests/arm/32/tests_asm.o \
	tests/arm/64/tests.o \
	tests/arm/64/tests_asm.o \
	tests/riscv/tests.o \
	tests/riscv/tests_asm.o \
	tests/x86/tests.o
NASM_OBJS = \
	src/x86/checkasm.o \
	tests/x86/tests_asm.o

NASM_FMT ?=
ifdef NASM_FMT
OBJS += $(NASM_OBJS)
endif

# TODO: Keep sources and objects in separate directories?

$(EXE): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.asm
	nasm -f $(NASM_FMT) $(CPPFLAGS) -o $@ $<

clean:
	$(RM) $(EXE) $(OBJS) $(NASM_OBJS)
