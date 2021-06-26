subdir('icons')
subdir('appdata')

mime = 'org.gnome.Notes.xml'

i18n.merge_file(
  input: mime + '.in',
  output: mime,
  po_dir: po_dir,
  data_dirs: its_dir,
  install: true,
  install_dir: join_paths(bijiben_datadir, 'mime', 'packages')
)

desktop = 'org.gnome.Notes.desktop'
desktop_conf = configuration_data()
desktop_conf.set ('icon', application_id)

i18n.merge_file(
  type: 'desktop',
  input: configure_file(
    input: files (desktop + '.in'),
    output: desktop + '.in',
    configuration: desktop_conf
  ),
  output: '@0@.desktop'.format(application_id),
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
  'resources/style.css',
  'resources/note-symbolic.svg'
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

install_data(
  'org.gnome.Notes.gschema.xml',
  install_dir: join_paths(bijiben_datadir, 'glib-2.0', 'schemas')
)

gnome.compile_schemas(build_by_default: true)
