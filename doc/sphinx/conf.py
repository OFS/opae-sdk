# -*- coding: utf-8 -*-
#

import sys
import os
import shutil

# The suffix of source filenames.
from recommonmark.parser import CommonMarkParser
from recommonmark.transform import AutoStructify

# Invoke doxygen
import subprocess
subprocess.call('cd ../doxygen; doxygen', shell=True)

# Assure symbolic link is removed
if os.path.lexists('docs') and os.path.islink('docs'):
    os.remove('docs')
else:
    shutil.rmtree('docs', ignore_errors=True)

# Create symbolic link
if os.path.exists('../../src'):
    os.symlink('../../src', './docs')

# Default doxygen location
doxyDir = os.path.join(os.path.expanduser('~'),
                           'cmake_builds',
                           'cpt_sys_sw-fpga-sw',
                           'doc',
                           'xml')

# Assure symbolic link is removed?
if os.path.exists(doxyDir) and not os.path.exists('doxygen_xml'):
    os.symlink(doxyDir, './doxygen_xml')

# Enabled extensions
extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.intersphinx',
    'sphinx.ext.todo',
    'sphinx.ext.coverage',
    'breathe'
]

# Use breathe to include doxygen documents
breathe_projects = {'FPGA-API' : 'doxygen_xml/'}
breathe_default_project = 'FPGA-API'

# Markdown/Org-mode extension
# sys.path.append(os.path.abspath('_contrib'))
# extensions += ["sphinxcontrib_markdown"]

source_parsers = {'.md': CommonMarkParser}
source_suffix = ['.rst', '.md']

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# The encoding of source files.
source_encoding = 'utf-8-sig'

# The suffix of source filenames.
source_suffix = ['.md', '.rst']

# The master toctree document.
master_doc = 'index'

# General information about the project.
project = u'OPAE'
copyright = u'2017 Intel Corporation'
author = u'Intel DCG FPT SW'

# The version info for the project you're documenting
#
# The short X.Y version.
version = u'0.9.0'
# The full version, including alpha/beta/rc tags.
release = u'0.9.0'

# This is also used if you do content translation via gettext catalogs.
# Usually you set "language" from the command line for these cases.
language = 'en'

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# The default language to highlight source code in.
highlight_language = 'c'

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = 'sphinx'

# If true, `todo` and `todoList` produce output, else they produce nothing.
todo_include_todos = True

# -- Options for HTML output ---------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.

html_extra_path = ['../doxygen/build/html']

# Add any paths that contain custom themes here, relative to this directory.
html_theme_path = ['_themes']

import sphinx_rtd_theme
html_theme_path += [sphinx_rtd_theme.get_html_theme_path()]
html_theme = "sphinx_rtd_theme"

# Theme options are theme-specific and customize the look and feel of a theme
# further.  For a list of options available for each theme, see the
# documentation.
#html_theme_options = {}

# The name for this set of Sphinx documents.  If None, it defaults to
# "<project> v<release> documentation".
html_title = u'OPAE'

# A shorter title for the navigation bar.  Default is the same as html_title.
# html_short_title = u'OPAE'

# The name of an image file (relative to this directory) to place at the top
# of the sidebar.
# html_logo = "_static/intel_rgb_62.png"

# The name of an image file (relative to this directory) to use as a favicon of
# the docs.  This file should be a Windows icon file (.ico) being 16x16 or 32x32
# pixels large.
#html_favicon = None

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

# If not '', a 'Last updated on:' timestamp is inserted at every page bottom,
# using the given strftime format.
#html_last_updated_fmt = '%b %d, %Y'

# If true, SmartyPants will be used to convert quotes and dashes to
# typographically correct entities.
#html_use_smartypants = True

# Custom sidebar templates, maps document names to template names.
#html_sidebars = {}

# Additional templates that should be rendered to pages, maps page names to
# template names.
#html_additional_pages = {}

# If false, no module index is generated.
#html_domain_indices = True

# If false, no index is generated.
#html_use_index = True

# If true, the index is split into individual pages for each letter.
#html_split_index = False

# If true, links to the reST sources are added to the pages.
#html_show_sourcelink = True

# If true, "Created using Sphinx" is shown in the HTML footer. Default is True.
#html_show_sphinx = True

# If true, "(C) Copyright ..." is shown in the HTML footer. Default is True.
#html_show_copyright = True

# If true, an OpenSearch description file will be output, and all pages will
# contain a <link> tag referring to it.  The value of this option must be the
# base URL from which the finished HTML is served.
#html_use_opensearch = ''

# This is the file name suffix for HTML files (e.g. ".xhtml").
#html_file_suffix = None

# Language to be used for generating the HTML full-text search index.
# Sphinx supports the following languages:
#   'da', 'de', 'en', 'es', 'fi', 'fr', 'hu', 'it', 'ja'
#   'nl', 'no', 'pt', 'ro', 'ru', 'sv', 'tr'
#html_search_language = 'en'

# A dictionary with options for the search language support, empty by default.
# Now only 'ja' uses this config value
#html_search_options = {'type': 'default'}

# The name of a javascript file (relative to the configuration directory) that
# implements a search results scorer. If empty, the default will be used.
#html_search_scorer = 'scorer.js'

# Output file base name for HTML help builder.
htmlhelp_basename = 'IntelFPGADocumentation'

# -- Options for LaTeX output ---------------------------------------------

latex_elements = {
# The paper size ('letterpaper' or 'a4paper').
#'papersize': 'letterpaper',

# The font size ('10pt', '11pt' or '12pt').
#'pointsize': '10pt',

# Additional stuff for the LaTeX preamble.
#'preamble': '',

# Latex figure (float) alignment
#'figure_align': 'htbp',
}

# Grouping the document tree into LaTeX files. List of tuples
# (source start file, target name, title,
#  author, documentclass [howto, manual, or own class]).

# Split document toctrees
quick_start_doc = 'docs/fpga_api/quick_start/readme'
prog_guide_doc = 'docs/fpga_api/prog_guide/readme'
drv_arch_doc = 'docs/drv_arch/drv_arch'
#hssi_config_doc = 'docs/fpga_tools/hssi_config/readme'
#hssi_loopback_doc = 'docs/fpga_tools/hssi_loopback/readme'
hssi_tuner_doc = 'docs/fpga_tools/mhssi_tuner/readme'
alaska_fw_loader_doc = 'docs/fpga_tools/alaska_fw_loader/readme'
fpga_tools_doc = 'docs/fpga_tools/readme'
ase_userguide_doc = 'docs/ase_userguide/ase_userguide'
api_build_doc = 'docs/build_chain/fpga_api/api_build'
driver_build_doc = 'docs/build_chain/fpga_driver/driver_build'
install_guide_doc = 'docs/install_guide/installation_guide'


latex_documents = [
    (quick_start_doc, 'quick_start.tex', u'Intel FPGA Quick Start Guide', u'FPT SW Development Team', 'howto'),
    (prog_guide_doc, 'prog_guide.tex', u'Intel FPGA Programming Guide', u'FPT SW Development Team', 'howto'),
    (fpga_tools_doc, 'fpga_tools.tex', u'Intel FPGA Tools', u'FPT SW Development Team', 'howto'),
    # (fpgainfo_doc, 'fpgainfo.tex', u'fpgainfo', u'FPT SW Development Team', 'howto'),
    (ase_userguide_doc, 'ase_userguide.tex', u'Intel AFU Simulation Environment (ASE) User Guide', u'FPT SW Development Team', 'howto'),
    (api_build_doc, 'api_build.tex', u'apiBuild', u'FPT SW Development Team', 'howto'),
    (driver_build_doc, 'driver_build.tex', u'Building the Intel FPGA driver', u'FPT SW Development Team', 'howto'),
    (install_guide_doc, 'install_guide.tex', u'Intel FPGA Software Stack Installation Guide', u'FPT SW Development Team', 'howto'),
    (drv_arch_doc, 'drv_arch.tex', u'FPGA Driver Architecture', u'FPT SW Development Team', 'manual'),
    # (hssi_config_doc, 'hssi_config.tex', u'HSSI config manual', u'FPT SW Development Team', 'howto'),
    # (hssi_loopback_doc, 'hssi_loopback.tex', u'HSSI loopback manual', u'FPT SW Development Team', 'manual'),
    ]

# The name of an image file (relative to this directory) to place at the top of
# the title page.
#latex_logo = None

# For "manual" documents, if this is true, then toplevel headings are parts,
# not chapters.
#latex_use_parts = False

# If true, show page references after internal links.
#latex_show_pagerefs = False

# If true, show URL addresses after external links.
#latex_show_urls = False

# Documents to append as an appendix to all manuals.
#latex_appendices = []

# If false, no module index is generated.
#latex_domain_indices = True


# -- Options for manual page output ---------------------------------------

# One entry per manual page. List of tuples
# (source start file, name, description, authors, manual section).
man_pages = [
#    (master_doc, 'intel-fpga', u'Intel FPGA Documentation',
#     [author], 1),
    ("docs/fpga_tools/coreidle/coreidle", 'coreidle', u'Adjust number of active cores to account for FPGA power consumption', [author], 8),
    ("docs/fpga_tools/error_monitor/fpga_errors", 'fpgaerr', u'Error reporting and clearing', [author], 8),
    ("docs/fpga_tools/fpgaconf/fpgaconf", 'fpgaconf', u'Configure green bitstreams to an FPGA', [author], 8),
    ("docs/fpga_tools/fpgad/fpgad", 'fpgad', u'Log errors and generate events', [author], 8),
    ("docs/fpga_tools/fpgadiag/README", 'fpgadiag', u'FPGA diagnosis and testing tool', [author], 8),
    ("docs/fpga_tools/fpgainfo/fpgainfo", 'fpgainfo', u'FPGA information tool', [author], 8),
    ("docs/fpga_tools/fpgamux/fpgamux", 'fpgamux', u'Software MUX for running multiple AFU tests in one GBS', [author], 8),
    ("docs/fpga_tools/hssi_config/readme", 'hssi_config', u'Read from or write to HSSI registers', [author], 8),
    ("docs/fpga_tools/hssi_loopback/readme", 'hssi_loopback', u'Interact with a packet generator GBS', [author], 8),
    ("docs/fpga_tools/mmlink/mmlink", 'mmlink', u'Enable remote SignalTAP debugging', [author], 8),
    ("docs/fpga_tools/thermal_power_monitor/power", 'fpgapwr', u'Query power consumed by FPGA', [author], 8),
    ("docs/fpga_tools/thermal_power_monitor/temp", 'fpgatmp', u'Query FPGA temperature readings', [author], 8),
    ("docs/fpga_tools/userclk/userclk", 'userclk', u'Set AFU high and low clock frequency', [author], 8)
]

# If true, show URL addresses after external links.
#man_show_urls = False

# Example configuration for intersphinx: refer to the Python standard library.
intersphinx_mapping = {'https://docs.python.org/': None}

# app setup hook to enable AutoStructify (for ```eval_rst blocks)
def setup(app):
    app.add_config_value('recommonmark_config', {
        'enable_auto_toc_tree': False,
        'enable_auto_doc_ref': False,
        'enable_math': False,
        'enable_inline_math': False,
        'enable_eval_rst': True,
    }, True)
    app.add_transform(AutoStructify)
