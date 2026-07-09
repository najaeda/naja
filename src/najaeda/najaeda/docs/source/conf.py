# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import os
import re
import sys

# Add the src directory to sys.path
sys.path.insert(0, os.path.abspath('../../../'))


def read_naja_release():
    docs_source_dir = os.path.abspath(os.path.dirname(__file__))
    repo_root = os.path.abspath(os.path.join(docs_source_dir, '../../../../..'))
    version_file = os.path.join(repo_root, 'src/core/NajaVersion.h.in')
    with open(version_file, encoding='utf-8') as stream:
        content = stream.read()
    match = re.search(r'NAJA_VERSION\s*\{\s*"(?P<value>[^"]+)"\s*\}', content)
    if not match:
        raise RuntimeError(f'Cannot read NAJA_VERSION from {version_file}')
    return match.group('value')

project = 'najaeda'
copyright = '2024, Naja authors'
author = 'Naja authors'
release = read_naja_release()

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.autodoc',       # Enables the automodule directive
    'sphinx.ext.napoleon',      # (Optional) Supports Google and NumPy-style docstrings
    'sphinx.ext.viewcode',      # (Optional) Links to source code in docs
    'sphinx.ext.todo',          # (Optional) For TODOs in the documentation
]

try:
    from najaeda import naja as raw_naja
    sys.modules.setdefault("najaeda.naja", raw_naja)
    tags.add("raw_naja_available")
    autodoc_mock_imports = []
except Exception:
    autodoc_mock_imports = ["naja", "najaeda.naja"]

templates_path = ['_templates']
exclude_patterns = []
suppress_warnings = ["autodoc.mocked_object"]

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'

# Run preprocessing step if building in a specific environment
import os
print("Running preprocessing script for Sphinx documentation...")
preprocessor_script = os.path.abspath('./preprocessor.py')
source_dir = os.path.abspath('../../../examples')
source_rst = os.path.abspath('./examples.rst.in')
dest_rst = os.path.abspath('./examples.rst')

try:
    import subprocess
    subprocess.call([
        sys.executable, preprocessor_script, 
        '--source_dir', source_dir, 
        '--source_rst', source_rst, 
        '--dest_rst', dest_rst
    ])
    print("Preprocessing completed successfully.")
except Exception as e:
    print(f"Error during preprocessing: {e}")
