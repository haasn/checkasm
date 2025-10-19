# Copyright © 2025 Niklas Haas
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

checkasm_test_sources = files(
  'checkasm.c',
  'generic.c'
)

if arch_x86
  checkasm_test_sources += files(
    'x86/tests.asm',
    'x86/tests.c',
  )
elif arch_rv64
  checkasm_test_sources += files(
    'riscv/tests.S',
    'riscv/tests.c',
  )
else
  warning('There are no assembly functions to test on the current platform.')
endif

checkasm_test = executable('checkasm',
  checkasm_test_sources,
  checkasm_config,
  c_args: ['-Wno-unused-parameter', '-Wno-unused-local-typedef'],
  objects: checkasm_library.extract_all_objects(recursive: true),
  # Also link against checkasm dependencies to make extracted objects happy
  dependencies: checkasm_dependencies + checkasm_dep,
  build_by_default: false,
  install: false,
)

test('checkasm',      checkasm_test, suite: 'checkasm', args: ['--verbose'])
benchmark('checkasm', checkasm_test, suite: 'checkasm', args: ['--bench', '--verbose'])
