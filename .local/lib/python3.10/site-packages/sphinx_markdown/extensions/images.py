"""Markdown extension for prepending _static to images
"""

import os

from markdown.extensions import Extension
from markdown.treeprocessors import Treeprocessor


class StaticImagesProcessor(Treeprocessor):
    """Extended markdown to prepend images with the _static directory
    """
    def __init__(self, markdown_instance, static_dir):
        self.static_dir = static_dir
        Treeprocessor.__init__(self, markdown_instance)

    def run(self, root):
        """Do replacement
        """
        for image in root.getiterator('img'):
            if image.attrib['src'][0] == '/':
                continue
            image.attrib['src'] = os.path.join(self.static_dir, image.attrib['src'])


class StaticImagesExtension(Extension):
    """Markdown Extension for StaticImagesProcessor
    """
    def __init__(self, **kwargs):
        self.config = {'static_dir': ['_static', 'Static directory']}
        Extension.__init__(self, **kwargs)

    def extendMarkdown(self, markdown_instance, md_globals):
        """Markdown extension entrypoint
        """
        _ = md_globals
        treep = StaticImagesProcessor(markdown_instance, self.getConfig('static_dir'))
        markdown_instance.treeprocessors.add('sphinx_images', treep, '_end')
