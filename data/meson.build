subdir('icons')

mime = 'org.gnome.bijiben.xml'

custom_target(
  mime,
  input: mime + '.in',
  output: mime,
  command: intltool_xml_cmd,
  install: true,
  install_dir: join_paths(bijiben_datadir, 'mime', 'packages')
)

appdata = 'org.gnome.bijiben.appdata.xml'

custom_target(
  appdata,
  input: appdata + '.in',
  output: appdata,
  command: intltool_xml_cmd,
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

custom_target(
  desktop,
  input: desktop_in,
  output: desktop,
  command: intltool_desktop_cmd,
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
