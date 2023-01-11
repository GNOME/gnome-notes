sources = files(
  'deserializer/biji-lazy-deserializer.c',
  'deserializer/biji-tomboy-reader.c',
  'editor/biji-editor-selection.c',
  'editor/biji-webkit-editor.c',
  '../items/bjb-item.c',
  'provider/biji-import-provider.c',
  'provider/biji-local-note.c',
  'provider/biji-local-provider.c',
  'provider/biji-memo-note.c',
  'provider/biji-memo-provider.c',
  'provider/biji-nextcloud-note.c',
  'provider/biji-nextcloud-provider.c',
  'provider/biji-provider.c',
  'serializer/biji-lazy-serializer.c',
  'biji-date-time.c',
  'biji-info-set.c',
  'biji-manager.c',
  'biji-notebook.c',
  'biji-note-obj.c',
  'biji-string.c',
  'biji-timeout.c',
  'biji-tracker.c',
  '../bjb-utils.c',
)

marshalers = gnome.genmarshal(
  'biji-marshalers',
  sources: 'biji-marshalers.list',
  prefix: '_biji_marshal'
)

deps = bijiben_deps

cflags = []

libbiji = static_library(
  'biji',
  sources: sources + marshalers,
  include_directories: top_inc,
  dependencies: deps,
  c_args: cflags
)

libbiji_dep = declare_dependency(
  link_with: libbiji,
  include_directories: include_directories('.'),
  dependencies: deps,
  sources: marshalers[1],
)
