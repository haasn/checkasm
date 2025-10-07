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

# Dependencies
libm_dependency     = cc.find_library('m', required: false)
rt_dependency       = cc.find_library('rt', required: false)
libdl_dependency    = cc.find_library('dl', required: false)
thread_dependency   = dependency('threads')

have_pthread_np = cc.check_header('pthread_np.h')
have_pthread_setaffinity_np = cc.has_function('pthread_setaffinity_np',
  prefix: [
    '#include <pthread.h>',
    have_pthread_np ? '#include <pthread_np.h>' : ''
  ],
  args: test_args,
  dependencies: thread_dependency,
)

have_getauxval = false
have_elf_aux_info = false
if (host_machine.cpu_family() == 'aarch64' or
    host_machine.cpu_family().startswith('arm') or
    host_machine.cpu_family().startswith('loongarch') or
    host_machine.cpu() == 'ppc64le' or
    host_machine.cpu_family().startswith('riscv'))
    have_getauxval = cc.has_function('getauxval', prefix : '#include <sys/auxv.h>', args : test_args)
    have_elf_aux_info = cc.has_function('elf_aux_info', prefix : '#include <sys/auxv.h>', args : test_args)
endif

# Build configuration
cdata = configuration_data()
cdata.set('PREFIX',                         cc.symbols_have_underscore_prefix())
cdata.set10('CONFIG_LINUX_PERF',            get_option('linux_perf'))
cdata.set10('CONFIG_MACOS_KPERF',           get_option('macos_kperf'))
cdata.set10('ARCH_AARCH64',                 arch_aarch64)
cdata.set10('ARCH_ARM',                     arch_arm)
cdata.set10('ARCH_X86',                     arch_x86)
cdata.set10('ARCH_X86_64',                  arch_x86_64)
cdata.set10('ARCH_X86_32',                  arch_x86_32)
cdata.set10('ARCH_PPC64LE',                 arch_ppc64le)
cdata.set10('ARCH_RISCV',                   arch_riscv)
cdata.set10('ARCH_RV32',                    arch_rv32)
cdata.set10('ARCH_RV64',                    arch_rv64)
cdata.set10('ARCH_LOONGARCH',               arch_loongarch)
cdata.set10('ARCH_LOONGARCH32',             arch_loongarch32)
cdata.set10('ARCH_LOONGARCH64',             arch_loongarch64)
cdata.set10('HAVE_UNISTD_H',                cc.check_header('unistd.h'))
cdata.set10('HAVE_PTHREAD_NP_H',            have_pthread_np)
cdata.set10('HAVE_PTHREAD_SETAFFINITY_NP',  have_pthread_setaffinity_np)
cdata.set10('HAVE_GETAUXVAL',               have_getauxval)
cdata.set10('HAVE_ELF_AUX_INFO',            have_elf_aux_info)

config_target = configure_file(
  output: 'config.h',
  configuration: cdata,
  install: false,
)

config_nasm_target = configure_file(
  output: 'config.asm',
  output_format: 'nasm',
  configuration: cdata,
  install: false,
)

# Compiler and assembler flags
checkasm_nasm_args = []

if arch_x86
  checkasm_nasm_args += '-DPIC=1'

  # Convert SSE asm into (128-bit) AVX when compiler flags are set to use AVX instructions
  if cc.get_define('__AVX__').strip() != ''
      checkasm_nasm_args += '-DFORCE_VEX_ENCODING=1'
  else
      checkasm_nasm_args += '-DFORCE_VEX_ENCODING=0'
  endif
endif

# Build definition
checkasm_sources = files(
  'checkasm.c',
  'cpu.c',
  'perf.c',
  'signal.c',
  'util.c',
)

if host_machine.cpu_family() == 'aarch64' or host_machine.cpu() == 'arm64'
  checkasm_sources += files(
    'arm/checkasm_64.S',
    'arm/cpu.c',
  )
elif host_machine.cpu_family().startswith('arm')
  checkasm_sources += files(
    'arm/checkasm_32.S',
    'arm/cpu.c',
  )
elif host_machine.cpu_family() == 'riscv64'
  checkasm_sources += files(
    'riscv/checkasm_64.S',
    'riscv/cpu.c',
    'riscv/64/cpu.S',
  )
elif host_machine.cpu_family().startswith('x86')
  checkasm_sources += files(
    'x86/checkasm.asm',
    'x86/cpu.c',
  )
elif host_machine.cpu_family().startswith('loongarch')
  checkasm_sources += files(
    'loongarch/checkasm.S',
    'loongarch/cpu.c',
  )
endif

checkasm_dependencies = [
  thread_dependency,
  rt_dependency,
  libdl_dependency,
  libm_dependency,
]

checkasm_library = library(meson.project_name(),
  checkasm_sources,
  config_target,
  config_nasm_target,
  include_directories: checkasm_inc_dirs,
  nasm_args: checkasm_nasm_args,
  dependencies: checkasm_dependencies,
  install: true,
)

checkasm_dep = declare_dependency(
  link_with: checkasm_library,
  include_directories: checkasm_inc_dirs,
)
