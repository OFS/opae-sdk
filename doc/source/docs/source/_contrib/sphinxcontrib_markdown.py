# -*- coding: utf-8 -*-
"""
    sphinxcontrib_markdown
    ~~~~~~~~~~~~~~~~~~~~~~~

    :copyright: Copyright 2012, 2017 by Takeshi Komiya.
    :license: BSDL.
    :changelog: Abelardo Jara - Added support for Org mode
"""

import os
from tempfile import mkstemp

class MarkdownProcessor(object):
    def on_builder_inited(self, app):
        orig_find_files = app.env.find_files

        def find_files(config, builder):
            orig_find_files(config, builder)
            app.env.found_docs.add('index')

        app.env.find_files = find_files

    def on_env_purge_doc(self, app, env, docname):
        env.find_files = None
        del env.find_files

    def on_source_read(self, app, docname, source):
        if docname == 'index':
            return

        try:
            input = mkstemp()
            output = mkstemp()
            os.close(input[0])
            os.close(output[0])

            with open(input[1], 'wt') as f:
                f.write(source[0].encode('utf-8'))

            if "_org" in docname:
                cmdline = "pandoc -r org -w rst %s -o %s" % (input[1], output[1])
                os.system(cmdline)
            else:
                cmdline = "pandoc -r markdown -w rst %s -o %s" % (input[1], output[1])
                os.system(cmdline)
                cmdline = "sed -ie 's/^:://g' {}".format(output[1])
                os.system(cmdline)
                cmdline = "sed -ie 's/code::/code-block::/g' {}".format(output[1])
                os.system(cmdline)
                cmdline = "sed -ie 's/   :caption/:caption/g' {}".format(output[1])
                os.system(cmdline)
                cmdline = "sed -ie 's/   :name/:name/g' {}".format(output[1])
                os.system(cmdline)
                cmdline = "sed -ie 's/   :dedent/:dedent/g' {}".format(output[1])
                os.system(cmdline)

            os.system(cmdline)
            source[0] = open(output[1]).read().decode('utf-8')
        finally:
            os.unlink(input[1])
            os.unlink(output[1])

    def setup(self, app):
        app.add_config_value('markdown_title', None, 'html')

        app.connect('builder-inited', self.on_builder_inited)
        app.connect('env-purge-doc', self.on_env_purge_doc)
        app.connect('source-read', self.on_source_read)

def setup(app):
    md = MarkdownProcessor()
    md.setup(app)
