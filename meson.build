project('checkasm_example',
  ['c', 'nasm'],
  default_options : ['warning_level=3'])

nihcpy_sources = files('src/nihcpy.c')

if host_machine.cpu_family() == 'x86_64'
  nihcpy_sources += [
    files('src/x86/nihcpy_x86.asm')
  ]
endif

nihcpy = library('nihcpy', nihcpy_sources,
  install : false)
