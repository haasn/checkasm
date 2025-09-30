# Copyright © 2024, Marvin Scholz
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


# This is an example project that illustrates how checkasm would be integrated
# and used to test a library, in this example the nihcpy library.
project('checkasm_example',
  ['c', 'nasm'],
  default_options: ['warning_level=3'])

if host_machine.system() in ['linux', 'gnu', 'emscripten']
  add_project_arguments('-D_GNU_SOURCE', language: 'c')
endif

if get_option('b_sanitize') == 'memory'
    error('asm causes false positives with the memory sanitizer.')
endif

# Include build definition for checkasm
# For details see the checkasm/meson.build file
subdir('checkasm')

# This is the library we aim to test with checkasm in this project,
# in an actual usecase this would be your library instead.
example_sources = files('example/nihcpy.c')

if host_machine.cpu_family() == 'x86_64'
  example_sources += [
    files('example/x86/nihcpy_x86.asm')
  ]
else
  warning('Not building for x86_64, so there are no assembly functions to test.')
endif

example = library('example', example_sources,
  install: false)

# Checkasm test for the example library
example_checkasm_sources = files('example/checkasm.c', 'example/test_nihcpy.c')

example_checkasm = executable('example_checkasm',
  checkasm_sources + example_checkasm_sources,

  c_args: checkasm_c_args,
  nasm_args: checkasm_nasm_args,

  objects: [
    example.extract_all_objects(recursive: true),
  ],

  build_by_default: true,
  dependencies: checkasm_dependencies,
)

test('checkasm', example_checkasm, suite: 'checkasm', timeout: 180)
benchmark('checkasm', example_checkasm, suite: 'checkasm', timeout: 3600, args: '--bench')
