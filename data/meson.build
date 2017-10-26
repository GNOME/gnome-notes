subdir('icons')

mime = 'org.gnome.bijiben.xml'

i18n.merge_file(
  mime,
  input: mime + '.in',
  output: mime,
  po_dir: po_dir,
  data_dirs: its_dir,
  install: true,
  install_dir: join_paths(bijiben_datadir, 'mime', 'packages')
)

appdata = 'org.gnome.bijiben.appdata.xml'

i18n.merge_file(
  appdata,
  input: appdata + '.in',
  output: appdata,
  po_dir: po_dir,
  install: true,
  install_dir: join_paths(bijiben_datadir, 'metainfo')
)

desktop = 'org.gnome.bijiben.desktop'

desktop_conf = configuration_data()
desktop_conf.set('VERSION', bijiben_version)

desktop_in = configure_file(
  input: desktop + '.in.in',
  output: desktop + '.in',
  configuration: desktop_conf
)

i18n.merge_file(
  desktop,
  type: 'desktop',
  input: desktop_in,
  output: desktop,
  po_dir: po_dir,
  install: true,
  install_dir: join_paths(bijiben_datadir, 'applications')
)

service_conf = configuration_data()
service_conf.set('libexecdir', bijiben_libexecdir)

service = 'org.gnome.bijiben.SearchProvider.service'

configure_file(
  input: service + '.in',
  output: service,
  install: true,
  install_dir: join_paths(bijiben_datadir, 'dbus-1', 'services'),
  configuration: service_conf
)

install_data(
  'org.gnome.bijiben-search-provider.ini',
  install_dir: join_paths(bijiben_datadir, 'gnome-shell', 'search-providers')
)

resource_data = files(
  'resources/Adwaita.css',
  'resources/app-menu.ui',
  'resources/editor-toolbar.ui',
  'resources/empty-results-box.ui',
  'resources/note-symbolic.svg',
  'resources/settings-dialog.ui',
  'resources/thumbnail-frame.png'
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
  'org.gnome.bijiben.gschema.xml',
  install_dir: join_paths(bijiben_datadir, 'glib-2.0', 'schemas')
)
