
import os

from docutils import nodes
import markdown

from sphinx_markdown.extensions.images import StaticImagesExtension


class MarkdownNode(nodes.raw, nodes.Element):
    """HTML container for markdown contents
    """
    filename = ''
    htmlcontent = ''
    extensions = []

    def load_markdown(self):
        """Save the markdown contents to this node
        """
        with open(self.filename) as handle:
            handle.readline()
            text = unicode(handle.read().decode('utf-8'))
        static_dir = os.path.relpath('_static',
                                     start=os.path.dirname(self.filename))
        sphinx_md_ext = StaticImagesExtension(static_dir=static_dir)
        extensions = [sphinx_md_ext]+self.extensions
        self.htmlcontent = markdown.markdown(text, extensions=extensions)

    def astext(self):
        return self.htmlcontent


def visit_markdown_node(document, node):
    document.body.append(node.htmlcontent)


def depart_markdown_node(document_, node_):
    pass
