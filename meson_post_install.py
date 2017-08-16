#!/usr/bin/env python3

import glob
import os
import re
import subprocess
import sys

if not os.environ.get('DESTDIR'):
  datadir = sys.argv[1]

  name_pattern = re.compile('hicolor_(?:apps|actions)_(?:\d+x\d+|scalable)_(.*)')
  search_pattern = '/**/hicolor_*'

  actions_icondir = os.path.join(datadir, 'bijiben', 'icons', 'hicolor')
  [os.rename(file, os.path.join(os.path.dirname(file), name_pattern.search(file).group(1)))
   for file in glob.glob(actions_icondir + search_pattern, recursive=True)]

  icondir = os.path.join(datadir, 'icons', 'hicolor')
  [os.rename(file, os.path.join(os.path.dirname(file), name_pattern.search(file).group(1)))
   for file in glob.glob(icondir + search_pattern, recursive=True)]

  print('Update icon cache...')
  subprocess.call(['gtk-update-icon-cache', '-f', '-t', icondir])

  schemadir = os.path.join(datadir, 'glib-2.0', 'schemas')
  print('Compile gsettings schemas...')
  subprocess.call(['glib-compile-schemas', schemadir])

  desktop_file = os.path.join(datadir, 'applications', 'org.gnome.bijiben.desktop')
  print('Validate desktop file...')
  subprocess.call(['desktop-file-validate', desktop_file])

  if sys.argv[2] == 'update-mimedb':
    mimedir = os.path.join(datadir, 'mime')
    print('Update mime database...')
    subprocess.call(['update-mime-database', mimedir])
