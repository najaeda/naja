# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import sys
import os

# Add the src directory to sys.path
sys.path.insert(0, os.path.abspath('../../../'))

project = 'najaeda'
copyright = '2024, Naja authors'
author = 'Naja authors'
release = '0.1.22'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.autodoc',       # Enables the automodule directive
    'sphinx.ext.napoleon',      # (Optional) Supports Google and NumPy-style docstrings
    'sphinx.ext.viewcode',      # (Optional) Links to source code in docs
    'sphinx.ext.todo',          # (Optional) For TODOs in the documentation
]

autodoc_mock_imports = ["najaeda.naja"]
templates_path = ['_templates']
exclude_patterns = []

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
        'python', preprocessor_script, 
        '--source_dir', source_dir, 
        '--source_rst', source_rst, 
        '--dest_rst', dest_rst
    ])
    print("Preprocessing completed successfully.")
except Exception as e:
    print(f"Error during preprocessing: {e}")