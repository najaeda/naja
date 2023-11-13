#...
extensions = [
  "breathe",
  'sphinx_rtd_theme',
]
#...

project = 'Naja'
copyright = '2023, The Naja authors'
author = 'The Naja authors'

html_theme = "sphinx_rtd_theme"
html_title = "Naja Documentation"

breathe_default_project = "naja"

import os
if 'READ_THE_DOCS_CONTEXT' in os.environ:
  import subprocess
  #call doxygen from cmake
  subprocess.call('mkdir build', shell=True)
  subprocess.call('cd build; cmake ../.. -DBUILD_ONLY_DOC=ON', shell=True)
  subprocess.call('cd build; make docs', shell=True)
  subprocess.call('cd build/docs; pwd; ls -all', shell=True)

breathe_projects = { "naja" : "./build/docs/xml/" }
breathe_default_project = "naja"
