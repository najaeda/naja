#...
extensions = [
  "breathe",
  'sphinx_rtd_theme',
]
#...

html_theme = "sphinx_rtd_theme"

breathe_default_project = "naja"

import subprocess
subprocess.call('doxygen', shell=True)