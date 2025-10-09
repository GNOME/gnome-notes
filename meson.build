project(
  'bijiben', 'c',
  version: '41.alpha',
  license: 'GPL3',
  default_options: 'buildtype=debugoptimized',
  meson_version: '>= 0.56.0'
)

bijiben_name = meson.project_name()
bijiben_version = meson.project_version()

bijiben_prefix = get_option('prefix')
bijiben_bindir = join_paths(bijiben_prefix, get_option('bindir'))
bijiben_libexecdir = join_paths(bijiben_prefix, get_option('libexecdir'))
bijiben_localedir = join_paths(bijiben_prefix, get_option('localedir'))
bijiben_datadir = join_paths(bijiben_prefix, get_option('datadir'))

bijiben_pkgdatadir = join_paths(bijiben_datadir, bijiben_name)

cc = meson.get_compiler('c')

config_h = configuration_data()

if get_option('profile') == 'development'
    bijiben_profile = '.Devel'
else
    bijiben_profile = ''
endif

application_id = 'org.gnome.Notes@0@'.format(bijiben_profile)

# defines
set_defines = [
  # package
  ['BIJIBEN_APPLICATION_ID', application_id],
  ['PACKAGE', bijiben_name],
  ['PACKAGE_BUGREPORT', 'https://gitlab.gnome.org/GNOME/gnome-notes/issues'],
  ['PACKAGE_NAME', bijiben_name],
  ['PACKAGE_STRING', '@0@ @1@'.format(bijiben_name, bijiben_version)],
  ['PACKAGE_TARNAME', bijiben_name],
  ['PACKAGE_URL', 'https://wiki.gnome.org/Apps/Notes'],
  ['PACKAGE_VERSION', bijiben_version],
  ['PROFILE', bijiben_profile],
  ['VERSION', bijiben_version],
  # i18n
  ['GETTEXT_PACKAGE', bijiben_name]
]

foreach define: set_defines
  config_h.set_quoted(define[0], define[1])
endforeach

# compiler flags
common_flags = [
  '-DHAVE_CONFIG_H',
  '-DDATADIR="@0@"'.format(bijiben_datadir),
  '-DPACKAGE_LOCALE_DIR="@0@"'.format(bijiben_localedir),
  '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_54',
]

if get_option('buildtype').contains('debug')
  test_cflags = [
    '-fno-strict-aliasing',
    '-Wcast-align',
    '-Wformat=2',
    '-Winit-self',
    '-Winline',
    '-Wmissing-declarations',
    '-Wmissing-format-attribute',
    '-Wmissing-include-dirs',
    '-Wmissing-noreturn',
    '-Wmissing-prototypes',
    '-Wnested-externs',
    '-Wno-error=missing-field-initializers',
    '-Wno-error=unused-parameter',
    '-Wno-missing-field-initializers',
    '-Wno-suggest-attribute=format',
    '-Wno-unused-parameter',
    '-Wold-style-definition',
    '-Wpacked',
    '-Wpointer-arith',
    '-Wredundant-decls',
    '-Wshadow',
    '-Wstrict-prototypes',
    '-Wswitch-default',
    '-Wswitch-enum',
    '-Wundef',
    '-Wunused-but-set-variable',
    '-Wwrite-strings'
  ]

  common_flags += cc.get_supported_arguments(test_cflags)
endif

add_project_arguments(common_flags, language: 'c')

tracker_sparql_dep = dependency('tracker-sparql-3.0', required: false)
has_tracker3 = tracker_sparql_dep.found()

# Tracker
private_store = get_option('private_store') or has_tracker3
config_h.set('TRACKER_PRIVATE_STORE', private_store)

bijiben_deps = [
  tracker_sparql_dep,
  dependency('gio-unix-2.0'),
  dependency('glib-2.0', version: '>= 2.80'),
  dependency('goa-1.0'),
  dependency('gtk4', version: '>= 4.14.0'),
  dependency('json-glib-1.0'),
  dependency('libecal-2.0', version: '>= 3.45.1'),
  dependency('libedataserver-1.2', version: '>= 3.45.1'),
  dependency('libadwaita-1', version: '>= 1.2.0'),
  dependency('libxml-2.0'),
  dependency('libcurl'),
  dependency('libsoup-3.0'),
  dependency('webkitgtk-6.0', version: '>= 2.40'),
  cc.find_library('m')
]

gnome = import('gnome')
i18n = import('i18n')

data_dir = join_paths(meson.project_source_root(), 'data')
its_dir = join_paths(meson.project_source_root(), 'gettext')
po_dir = join_paths(meson.project_source_root(), 'po')

top_inc = include_directories('.')
src_inc = include_directories('src')

subdir('data')
subdir('src')
subdir('po')
subdir('help')

configure_file(
  output: 'config.h',
  configuration: config_h
)

run_conf = configuration_data()
run_conf.set('BUILD_DIR', meson.current_build_dir())
configure_file(
  input: 'run.in',
  output: 'run',
  configuration: run_conf,
)

meson.add_install_script(
  'build-aux/meson_post_install.py',
  bijiben_datadir,
  get_option('update_mimedb') ? 'update_mimedb' : ''
)

output = '  Version:          ' + bijiben_version + '\n'
output += '  Use Private Store ' + private_store.to_string() + ' (Tracker3: ' + has_tracker3.to_string() + ')\n'
message(output)
