project('mosquittomainloop', 'c')

mosquitto = meson.get_compiler('c').find_library('mosquitto')

deps = [ dependency('glib-2.0'), mosquitto ]
inc = include_directories('./')
src = [ 'mosquittomainloop.c' ]
lib = static_library('mosquittomainloop', src,
                     dependencies : deps)

mosquittomainloop_dep = declare_dependency(include_directories : inc, link_with : lib)