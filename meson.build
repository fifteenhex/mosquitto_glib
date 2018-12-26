project('mosquitto_glib', 'c')

mosquitto = meson.get_compiler('c').find_library('mosquitto')

conf_data = configuration_data()
conf_data.set('debug', get_option('mosquitto_glib_debug') ? 1 : 0)
configure_file(input : 'config.h.in',
               output : 'config.h',
               configuration : conf_data)

deps = [ dependency('glib-2.0'), dependency('gobject-2.0'), mosquitto ]
inc = include_directories('./')
src = [ 'mosquitto_client.c' ]
lib = static_library('mosquitto_glib', src,
                     dependencies : deps)

mosquitto_glib_dep = declare_dependency(include_directories : inc, link_with : lib)