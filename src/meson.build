revision_tag = vcs_tag(
  input: 'version.h.in',
  output: 'version.h',
)
bijiben_deps += declare_dependency(sources: revision_tag)

subdir('libbiji')

sources = files(
  'bjb-log.c',
  'bjb-application.c',
  'bjb-color-button.c',
  'bjb-controller.c',
  'bjb-editor-toolbar.c',
  'bjb-import-dialog.c',
  'bjb-list-view-row.c',
  'bjb-main.c',
  'bjb-note-list.c',
  'bjb-note-view.c',
  'bjb-notebook-row.c',
  'bjb-notebooks-dialog.c',
  'bjb-settings.c',
  'bjb-settings-dialog.c',
  'bjb-share.c',
  'bjb-window.c',
  # backport of GTK4 listmodels
  'contrib/gtktypebuiltins.c',
  'contrib/gtkflattenlistmodel.c',
  'contrib/gtkrbtree.c',
  'contrib/gtkfilter.c',
  'contrib/gtkcustomfilter.c',
  'contrib/gtkfilterlistmodel.c',
  'contrib/gtksorter.c',
  'contrib/gtkcustomsorter.c',
  'contrib/gtksortlistmodel.c',
  'contrib/gtkslicelistmodel.c',
)

sources += gnome.compile_resources(
  'bjb-resources',
  gresource,
  c_name: 'bjb',
  source_dir: data_dir,
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
