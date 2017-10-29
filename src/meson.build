subdir('libbiji')

sources = files(
  'bjb-app-menu.c',
  'bjb-application.c',
  'bjb-color-button.c',
  'bjb-controller.c',
  'bjb-debug.c',
  'bjb-editor-toolbar.c',
  'bjb-empty-results-box.c',
  'bjb-import-dialog.c',
  'bjb-load-more-button.c',
  'bjb-main.c',
  'bjb-main-toolbar.c',
  'bjb-main-view.c',
  'bjb-note-view.c',
  'bjb-organize-dialog.c',
  'bjb-search-toolbar.c',
  'bjb-selection-toolbar.c',
  'bjb-settings.c',
  'bjb-settings-dialog.c',
  'bjb-share.c',
  'bjb-window-base.c'
)

resource_data = files(
  'resources/Adwaita.css',
  'resources/app-menu.ui',
  'resources/editor-toolbar.ui',
  'resources/note-symbolic.svg',
  'resources/settings-dialog.ui',
  'resources/thumbnail-frame.png'
)

sources += gnome.compile_resources(
  'bjb-resources',
  'bjb.gresource.xml',
  c_name: 'bjb',
  dependencies: resource_data,
  export: true
)

executable(
  bijiben_name,
  sources,
  include_directories: top_inc,
  dependencies: libbiji_dep,
  install: true,
  install_dir: bijiben_bindir
)

sources = files('bijiben-shell-search-provider.c')

sources += gnome.gdbus_codegen(
  'bijiben-shell-search-provider-generated',
  search_provider_dbus_iface,
  interface_prefix: 'org.gnome.',
  namespace: 'Bijiben'
)

executable(
  'bijiben-shell-search-provider',
  sources,
  include_directories: top_inc,
  dependencies: libbiji_dep,
  install: true,
  install_dir: bijiben_libexecdir
)
