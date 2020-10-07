apps_icons = [
  ['scalable', ('@0@.svg').format(application_id)],
  ['symbolic', 'org.gnome.Notes-symbolic.svg']
]

foreach icon: apps_icons
  install_data(
    join_paths('hicolor', icon[0], 'apps', icon[1]),
    install_dir: join_paths(bijiben_datadir, 'icons', 'hicolor', icon[0], 'apps')
  )
endforeach

actions_icons = [
  ['16x16', 'note.png'],
  ['24x24', 'note.png'],
  ['48x48', 'note.png'],
  ['symbolic', 'link-symbolic.svg']
]

foreach icon: actions_icons
  install_data(
    join_paths('hicolor', icon[0], 'actions', icon[1]),
    install_dir: join_paths(bijiben_pkgdatadir, 'icons', 'hicolor', icon[0], 'actions')
  )
endforeach
