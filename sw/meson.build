project( 'VirtualFTDI', ['c', 'cpp'] )
# 'ftdi/linux-x86_64',
inc = include_directories(['../modules/VirtualUSBDevice/Src', '../modules/VirtualUSBDevice/Lib', '../modules/acm-toolbox/sw', '../modules/jtag-utils/sw'])

udev = meson.get_compiler('cpp').find_library('udev')

#ftd2xxlib = meson.get_compiler('cpp').find_library('ftd2xx', dirs: ['ftjtag/sw/ftdi/linux-x86_64'], required: true)
#ftd2xxdep = declare_dependency(
#  link_args : [ '-L' + meson.current_source_dir() + '/ftjtag/sw/ftdi/linux-x86_64', '-lftd2xx'] )

srcs = ['VirtualFTDI.cpp', 'VirtualFTDIDevice.cpp', '../modules/VirtualUSBDevice/Src/USBIPLib.cpp', '../modules/jtag-utils/sw/JtagTap.cpp', '../modules/jtag-utils/sw/JtagAna.cpp', '../sw/FTEmul.cpp', '../modules/acm-toolbox/sw/fwComm.c', '../modules/acm-toolbox/sw/cmdXfer.c', ]

executable(
  'VirtualUSB',
  srcs, 
  include_directories: inc,
  dependencies: [udev]
)
