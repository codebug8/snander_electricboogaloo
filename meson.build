project('snander_electricboogaloo', 'c',
        default_options : ['c_std=c99'])

cc = meson.get_compiler('c')
cflags = cc.get_supported_arguments([
                                     '-Wall',
                                     '-Werror=incompatible-pointer-types',
                                     '-Werror=implicit-function-declaration',
                                     '-Werror=format-security'
                                    ])
add_project_arguments(cflags, language: 'c')
add_project_arguments('-D_GNU_SOURCE', language: 'c')

libdgpc_proj = subproject('libdgpc')
libdpgc_dep = libdgpc_proj.get_variable('libdgpc_dep')

libusrio_dep = dependency('libusrio', required : false)
if not libusrio_dep.found()
    libusrio_proj = subproject('libusrio')
    libusrio_dep = libusrio_proj.get_variable('libusrio_dep')
endif

libch341a_dep = dependency('libch341a', required : false)
if not libch341a_dep.found()
    libch341a_proj = subproject('libch341a')
    libch341a_dep = libch341a_proj.get_variable('libch341a_dep')
endif

subdir('src')

