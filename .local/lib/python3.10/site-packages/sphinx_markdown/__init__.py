"""Markdown file support for Sphinx
"""
from sphinx_markdown.nodes import (MarkdownNode, visit_markdown_node,
                                   depart_markdown_node)
from sphinx_markdown.directives import MarkdownIngestor
from sphinx_markdown.search import handle_page_context_html


def setup(app):
    """Sphinx Extension setup entrypoint
    """
    app.add_node(MarkdownNode,
                 html=(visit_markdown_node, depart_markdown_node))
    app.add_directive('markdown-ingest', MarkdownIngestor)
    app.add_config_value('markdown_search', default=True, rebuild='html')
    app.add_config_value('markdown_extensions', default=[], rebuild='env')
    app.connect('html-page-context', handle_page_context_html)
