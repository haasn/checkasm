project('checkasm_example',
  ['c', 'nasm'],
  default_options : ['warning_level=3'])

nihcpy = library('nihcpy',
  files(
    'src/nihcpy.c',
    'src/nihcpy_x86.asm'
  ),
  install : false)
