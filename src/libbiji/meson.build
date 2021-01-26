sources = files(
  'deserializer/biji-lazy-deserializer.c',
  'deserializer/biji-tomboy-reader.c',
  'editor/biji-editor-selection.c',
  'editor/biji-webkit-editor.c',
  'provider/biji-import-provider.c',
  'provider/biji-local-note.c',
  'provider/biji-local-provider.c',
  'provider/biji-memo-note.c',
  'provider/biji-memo-provider.c',
  'provider/biji-own-cloud-note.c',
  'provider/biji-own-cloud-provider.c',
  'provider/biji-provider.c',
  'serializer/biji-lazy-serializer.c',
  'biji-date-time.c',
  'biji-info-set.c',
  'biji-item.c',
  'biji-manager.c',
  'biji-notebook.c',
  'biji-note-obj.c',
  'biji-string.c',
  'biji-timeout.c',
  'biji-tracker.c',
  'biji-zeitgeist.c',
  '../bjb-utils.c',
)

marshalers = 'biji-marshalers'

sources += gnome.genmarshal(
  marshalers,
  sources: marshalers + '.list',
  prefix: '_biji_marshal'
)

deps = bijiben_deps

cflags = []

enable_zeitgeist = get_option('zeitgeist')
if enable_zeitgeist
  deps += [dependency('zeitgeist-2.0')]

  cflags += '-DBUILD_ZEITGEIST'
endif

libbiji = static_library(
  'biji',
  sources: sources,
  include_directories: top_inc,
  dependencies: deps,
  c_args: cflags
)

libbiji_dep = declare_dependency(
  link_with: libbiji,
  include_directories: include_directories('.'),
  dependencies: deps
)
