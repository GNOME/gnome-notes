apps_icon_sizes = [
  '16x16',
  '22x22',
  '24x24',
  '32x32',
  '48x48',
  '256x256'
]

# must be renamed to org.gnome.bijiben.png
foreach icon_size: apps_icon_sizes
  install_data(
    '_'.join(['hicolor', 'apps', icon_size, 'org.gnome.bijiben.png']),
    install_dir: join_paths(bijiben_datadir, 'icons', 'hicolor', icon_size, 'apps')
  )
endforeach

# must be renamed to org.gnome.bijiben-symbolic.svg
install_data(
  '_'.join(['hicolor', 'apps', 'scalable', 'org.gnome.bijiben-symbolic.svg']),
  install_dir: join_paths(bijiben_datadir, 'icons', 'hicolor', 'scalable', 'apps')
)

actions_icon_sizes = [
  '16x16',
  '24x24',
  '48x48'
]

# must be renamed to note.png
foreach icon_size: actions_icon_sizes
  install_data(
    '_'.join(['hicolor', 'actions', icon_size, 'note.png']),
    install_dir: join_paths(bijiben_pkgdatadir, 'icons', 'hicolor', icon_size, 'actions')
  )
endforeach

# must be renamed to link.svg
install_data(
  '_'.join(['hicolor', 'actions', 'scalable', 'link.svg']),
  install_dir: join_paths(bijiben_pkgdatadir, 'icons', 'hicolor', 'scalable', 'actions')
)
