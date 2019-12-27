subdir('icons')
subdir('appdata')

mime = 'org.gnome.Notes.xml'

i18n.merge_file(
  mime,
  input: mime + '.in',
  output: mime,
  po_dir: po_dir,
  data_dirs: its_dir,
  install: true,
  install_dir: join_paths(bijiben_datadir, 'mime', 'packages')
)

desktop = 'org.gnome.Notes.desktop'

i18n.merge_file(
  desktop,
  type: 'desktop',
  input: desktop + '.in',
  output: desktop,
  po_dir: po_dir,
  install: true,
  install_dir: join_paths(bijiben_datadir, 'applications')
)

service_conf = configuration_data()
service_conf.set('libexecdir', bijiben_libexecdir)

service = 'org.gnome.Notes.SearchProvider.service'

configure_file(
  input: service + '.in',
  output: service,
  install: true,
  install_dir: join_paths(bijiben_datadir, 'dbus-1', 'services'),
  configuration: service_conf
)

install_data(
  'org.gnome.Notes-search-provider.ini',
  install_dir: join_paths(bijiben_datadir, 'gnome-shell', 'search-providers')
)

resource_data = files(
  'resources/Adwaita.css',
  'resources/editor-toolbar.ui',
  'resources/empty-results-box.ui',
  'resources/note-symbolic.svg',
  'resources/settings-dialog.ui'
)

gresource = files('bjb.gresource.xml')

search_provider_dbus_iface = files('shell-search-provider-dbus-interfaces.xml')

web_files = files(
  'Default.css',
  'bijiben.js'
)

install_data(
  web_files,
  install_dir: bijiben_pkgdatadir
)

# https://github.com/mesonbuild/meson/issues/1687
custom_target('gsettings-enums',
  input: '../subprojects/libgd/libgd/gd-main-view.h',
  output: 'org.gnome.bijiben.enums.xml',
  install: true,
  install_dir: join_paths(bijiben_datadir, 'glib-2.0', 'schemas'),
  capture: true,
  command: ['glib-mkenums',
    '--comments', '<!-- @comment@ -->',
    '--fhead', '<schemalist>',
    '--vhead', '  <@type@ id="org.gnome.bijiben.@EnumName@">',
    '--vprod', '    <value nick="@valuenick@" value="@valuenum@"/>',
    '--vtail', '  </@type@>',
    '--ftail', '</schemalist>',
    '@INPUT@'
  ]
)
install_data(
  'org.gnome.Notes.gschema.xml',
  install_dir: join_paths(bijiben_datadir, 'glib-2.0', 'schemas')
)
