# Copyright © 2025 Niklas Haas
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

checkasm_api_headers = files(
  'attributes.h',
  'checkasm.h',
  'perf.h',
  'test.h',
)

if arch_x86_64
  osdep_header = files('osdep/x86_64.h')
elif arch_x86_32
  osdep_header = files('osdep/x86_32.h')
elif arch_arm
  osdep_header = files('osdep/arm.h')
elif arch_aarch64
  osdep_header = files('osdep/aarch64.h')
elif arch_riscv
  osdep_header = files('osdep/riscv.h')
elif arch_loongarch
  osdep_header = files('osdep/loongarch.h')
else
  osdep_header = files('osdep/generic.h')
endif

install_headers(checkasm_api_headers, subdir: meson.project_name())

configure_file(
  input: osdep_header,
  output: 'osdep.h',
  install: true,
  install_dir: get_option('includedir') / meson.project_name(),
  copy: true,
)

config_target = configure_file(output: 'config.h',
  configuration: cdata,
  install: true,
  install_dir: get_option('includedir') / meson.project_name(),
)
