# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'Una-Watch SDK'
copyright = '2026, Una-Watch Team'
author = 'Una-Watch Team'
release = '1.0'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.doctest',
    'sphinx.ext.intersphinx',
    'sphinx.ext.todo',
    'sphinx.ext.coverage',
    'sphinx.ext.imgmath',
    'sphinx.ext.mathjax',
    'sphinx.ext.ifconfig',
    'sphinx.ext.viewcode',
    'sphinx.ext.githubpages',
    'myst_parser',
    'sphinxcontrib.mermaid',
    'breathe',
    'sphinx_version_warning',
]

# MyST configuration
myst_enable_extensions = [
    "colon_fence",
    "html_image",
    "attrs_inline",
]
myst_linkify_fuzzy_links = False
myst_fence_as_directive = ["mermaid"]

# Breathe configuration
breathe_projects = {
    "SDK": "_build/doxygen/xml"
}
breathe_default_project = "SDK"

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

language = 'en'

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']
html_js_files = ['version-selector.js']

# -- Options for intersphinx extension ---------------------------------------
# https://www.sphinx-doc.org/en/master/usage/extensions/intersphinx.html#configuration

intersphinx_mapping = {
    'python': ('https://docs.python.org/3', None),
}

# -- Options for todo extension ----------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/extensions/todo.html#configuration

todo_include_todos = True

# -- Options for sphinx-version-warning extension ----------------------------
# https://sphinx-version-warning.readthedocs.io/en/latest/

versionwarning_messages = {
    "latest": {
        "message": "This is the latest version of the documentation.",
        "warning_type": "info",
    },
    "1.0": {
        "message": "This is version 1.0. Consider upgrading to the latest version.",
        "warning_type": "warning",
        "url": "https://una-watch.github.io/una-sdk/1.0/",
    },
    # Add more versions as needed
}

versionwarning_default_message = "This documentation is for an unsupported version. Please consider upgrading."
