project('mosquittomainloop', 'c')

mosquitto = meson.get_compiler('c').find_library('mosquitto')

deps = [ dependency('glib-2.0'), mosquitto ]
src = [ 'mosquittomainloop.c' ]
lib = static_library('mosquittomainloop', src,
                     dependencies : deps)