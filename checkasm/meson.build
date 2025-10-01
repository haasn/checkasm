# Copyright © 2018, VideoLAN and dav1d authors
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#
# Dependencies
# These dependencies are needed by checkasm
#
cc = meson.get_compiler('c')

libm_dependency     = cc.find_library('m', required: false)
rt_dependency       = cc.find_library('rt', required: false)
libdl_dependency    = cc.find_library('dl', required: false)
thread_dependency   = dependency('threads')

# Feature defines
checkasm_c_args = []
checkasm_nasm_args = []
test_args = []

if host_machine.system() in ['linux', 'gnu', 'emscripten']
    test_args += '-D_GNU_SOURCE'
endif

checkasm_defines = {
  'HAVE_UNISTD_H':        cc.check_header('unistd.h'),
  'HAVE_PTHREAD_NP_H':    cc.check_header('pthread_np.h'),
  'CONFIG_MACOS_KPERF':   get_option('macos_kperf'),
  'HAVE_C11_GENERIC':     cc.compiles('int x = _Generic(0, default: 0);', name: '_Generic', args: test_args),
  'ARCH_AARCH64':         host_machine.cpu_family() == 'aarch64' or host_machine.cpu() == 'arm64',
  'ARCH_ARM':             host_machine.cpu_family().startswith('arm') and host_machine.cpu() != 'arm64',
  'ARCH_X86':             host_machine.cpu_family().startswith('x86'),
  'ARCH_X86_64':          host_machine.cpu_family() == 'x86_64',
  'ARCH_X86_32':          host_machine.cpu_family() == 'x86',
  'ARCH_PPC64LE':         host_machine.cpu() == 'ppc64le',
  'ARCH_RISCV':           host_machine.cpu_family().startswith('riscv'),
  'ARCH_RV32':            host_machine.cpu_family() == 'riscv32',
  'ARCH_RV64':            host_machine.cpu_family() == 'riscv64',
  'ARCH_LOONGARCH':       host_machine.cpu_family().startswith('loongarch'),
  'ARCH_LOONGARCH32':     host_machine.cpu_family() == 'loongarch32',
  'ARCH_LOONGARCH64':     host_machine.cpu_family() == 'loongarch64',
}

if cc.symbols_have_underscore_prefix()
  checkasm_defines += { 'PREFIX' : true }
endif

have_pthread_setaffinity_np = cc.has_function('pthread_setaffinity_np',
  prefix: [
    '#include <pthread.h>',
    (checkasm_defines['HAVE_PTHREAD_NP_H']) ? '#include <pthread_np.h>' : ''
  ],
  args: test_args,
  dependencies: thread_dependency,
)

checkasm_defines += {
  'HAVE_PTHREAD_SETAFFINITY_NP': have_pthread_setaffinity_np
}

foreach define, value : checkasm_defines
  value_str = value.to_string('1', '0')
  checkasm_c_args += f'-D@define@=@value_str@'

  if host_machine.cpu_family().startswith('x86')
    if define in ['PREFIX', 'ARCH_X86_64', 'ARCH_X86_32']
      checkasm_nasm_args += f'-D@define@=@value_str@'
    endif
  endif
endforeach

if host_machine.cpu_family().startswith('x86')
  checkasm_nasm_args += '-DPIC=1'

  # Convert SSE asm into (128-bit) AVX when compiler flags are set to use AVX instructions
  if cc.get_define('__AVX__').strip() != ''
    checkasm_nasm_args += '-DFORCE_VEX_ENCODING=1'
  else
    checkasm_nasm_args += '-DFORCE_VEX_ENCODING=0'
  endif
endif


#
# Build definition for the checkasm executable
#

checkasm_sources = files(
  'checkasm.c',
  'common/cpu.c',
)

if host_machine.cpu_family() == 'aarch64' or host_machine.cpu() == 'arm64'
  checkasm_sources += files(
    'arm/checkasm_64.S',
    'common/arm/cpu.c',
  )
elif host_machine.cpu_family().startswith('arm')
  checkasm_sources += files(
    'arm/checkasm_32.S',
    'common/arm/cpu.c',
  )
elif host_machine.cpu_family() == 'riscv64'
  checkasm_sources += files(
    'riscv/checkasm_64.S',
    'common/riscv/cpu.c',
    'common/riscv/64/cpu.S',
  )
elif host_machine.cpu_family().startswith('x86')
  checkasm_sources += files(
    'x86/checkasm.asm',
    'common/x86/cpu.c',
  )
elif host_machine.cpu_family().startswith('loongarch')
  checkasm_sources += files(
    'loongarch/checkasm.S',
    'common/loongarch/cpu.c',
  )
endif

checkasm_dependencies = [
  thread_dependency,
  rt_dependency,
  libdl_dependency,
  libm_dependency,
]
