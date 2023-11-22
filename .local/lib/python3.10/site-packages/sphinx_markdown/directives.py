
from docutils.parsers.rst import directives
from sphinx.ext.autodoc import Options
from docutils.parsers.rst import Directive

from sphinx_markdown.nodes import MarkdownNode


class MarkdownIngestor(Directive):
    """Directive ``.. markdown-ingest::``
    """
    option_spec = {
        'filename': directives.unchanged,
    }

    def run(self):
        """Add a MarkdownNode when this directive is loaded
        """
        env = self.state.document.settings.env
        node = MarkdownNode()
        options = Options(self.options)
        node.filename = options.get('filename')
        node.extensions = env.config.markdown_extensions
        node.load_markdown()
        return [node]
