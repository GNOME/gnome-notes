project(
  'bijiben', 'c',
  version: '3.30.0',
  license: 'GPL3',
  default_options: 'buildtype=debugoptimized',
  meson_version: '>= 0.43.0'
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

# defines
set_defines = [
  # package
  ['PACKAGE', bijiben_name],
  ['PACKAGE_BUGREPORT', 'http://bugzilla.gnome.org/enter_bug.cgi?product=' + bijiben_name],
  ['PACKAGE_NAME', bijiben_name],
  ['PACKAGE_STRING', '@0@ @1@'.format(bijiben_name, bijiben_version)],
  ['PACKAGE_TARNAME', bijiben_name],
  ['PACKAGE_URL', 'https://wiki.gnome.org/Apps/Bijiben'],
  ['PACKAGE_VERSION', bijiben_version],
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
  '-DPACKAGE_LOCALE_DIR="@0@"'.format(bijiben_localedir)
]

if get_option('buildtype').contains('debug')
  test_cflags = [
    '-fno-strict-aliasing',
    '-Wcast-align',
    '-Wdeclaration-after-statement',
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

libgd = subproject(
  'libgd',
  default_options: [
    'static=true',
    'with-view-common=true',
    'with-gtk-hacks=true',
    'with-main-view=true',
    'with-tagged-entry=true'
  ]
)
libgd_dep = libgd.get_variable('libgd_dep')

tracker_sparql_dep = dependency('tracker-sparql-2.0', required: false)
if not tracker_sparql_dep.found()
  tracker_sparql_dep = dependency('tracker-sparql-1.0', required: false)
  if not tracker_sparql_dep.found()
    tracker_sparql_dep = dependency('tracker-sparql-0.18', required: true)
  endif
endif

bijiben_deps = [
  libgd_dep,
  tracker_sparql_dep,
  dependency('gio-unix-2.0'),
  dependency('glib-2.0', version: '>= 2.53.4'),
  dependency('goa-1.0'),
  dependency('gtk+-3.0', version: '>= 3.19.3'),
  dependency('libecal-1.2', version: '>= 3.13.90'),
  dependency('libedataserver-1.2', version: '>= 3.13.90'),
  dependency('libxml-2.0'),
  dependency('uuid'),
  dependency('webkit2gtk-4.0', version: '>= 2.10'),
  cc.find_library('m')
]

gnome = import('gnome')
i18n = import('i18n')

data_dir = join_paths(meson.source_root(), 'data')
its_dir = join_paths(meson.source_root(), 'gettext')
po_dir = join_paths(meson.source_root(), 'po')

top_inc = include_directories('.')

subdir('data')
subdir('src')
subdir('po')
subdir('help')

configure_file(
  output: 'config.h',
  configuration: config_h
)

meson.add_install_script(
  'build-aux/meson_post_install.py',
  bijiben_datadir,
  get_option('update_mimedb') ? 'update_mimedb' : ''
)

output = '  Version:          ' + bijiben_version + '\n'
output += '  Enable Zeitgeist  ' + enable_zeitgeist.to_string() + '\n'
message(output)
