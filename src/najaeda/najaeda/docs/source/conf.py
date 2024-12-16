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
release = '0.1.6'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.autodoc',       # Enables the automodule directive
    'sphinx.ext.napoleon',      # (Optional) Supports Google and NumPy-style docstrings
    'sphinx.ext.viewcode',      # (Optional) Links to source code in docs
    'sphinx.ext.todo',          # (Optional) For TODOs in the documentation
]

autodoc_mock_imports = ["najaeda.snl"]
templates_path = ['_templates']
exclude_patterns = []



# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'alabaster'
html_static_path = ['_static']
