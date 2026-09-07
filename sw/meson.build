project( 'VirtualFTDI', ['c', 'cpp', 'cython'], meson_version: '>=1.1' )

add_global_arguments( '-DCONFIG_WITH_COBS', language: ['c','cpp'] )

inc = include_directories(['../modules/VirtualUSBDevice/Src', '../modules/VirtualUSBDevice/Lib', '../modules/acm-toolbox/sw', '../modules/jtag-utils/sw', '../modules/cobs/sw'])

udev = meson.get_compiler('cpp').find_library('udev')

acm_toolbox_srcs = ['../modules/acm-toolbox/sw/fwComm.c', '../modules/acm-toolbox/sw/cmdXfer.c', '../modules/cobs/sw/cobsC.c']

srcs = ['VirtualFTDI.cpp', 'VirtualFTDIDevice.cpp', '../modules/VirtualUSBDevice/Src/USBIPLib.cpp', '../modules/jtag-utils/sw/JtagTap.cpp', '../modules/jtag-utils/sw/JtagAna.cpp', '../sw/FTEmul.cpp'] + acm_toolbox_srcs

executable(
  'VirtualFTDI',
  srcs, 
  include_directories: inc,
  dependencies: [udev]
)

if get_option('with-python')

py = import('python').find_installation(pure: false)

if ( py.found() )

py_ftemul_srcs = ['pyFTEmul.pyx', 'FTEmul.cpp'] + acm_toolbox_srcs
py.extension_module(
  'pyFTEmul',
  py_ftemul_srcs,
  override_options : ['cython_language=cpp'],
  include_directories : inc,
)

#py_ftemul_srcs = ['pyFTEmul.pyx', 'FTEmul.cpp'] + acm_toolbox_srcs
ftdirs=get_option('libftd2xx-dir')
if ftdirs == ''
  ftdirs=['/opt/ftdi/'+target_machine.system()+'-'+target_machine.cpu()]
endif
ftd2xxlib = meson.get_compiler('cpp').find_library('ftd2xx', dirs: ftdirs , required: false)

if (ftd2xxlib.found())

py.extension_module(
  'pyMPSSE',
  ['pyMPSSE.pyx'],
  override_options : ['cython_language=cpp'],
  include_directories : [inc, ftdirs],
  dependencies: [ftd2xxlib]
)

endif #libft2xx found

endif #python found

endif #option with-python enabled
