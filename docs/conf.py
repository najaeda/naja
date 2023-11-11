#...
extensions = [
  "breathe",
  'sphinx_rtd_theme',
]
#...

html_theme = "sphinx_rtd_theme"

breathe_default_project = "naja"

import subprocess
#call doxygen from cmake
subprocess.call('mkdir build', shell=True)
subprocess.call('cd build; cmake ../..', shell=True)
subprocess.call('cd build; make docs', shell=True)
