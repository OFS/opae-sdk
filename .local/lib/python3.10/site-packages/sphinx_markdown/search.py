"""Support for searching Markdown documents
"""

import os
import re

from docutils import nodes

from sphinx_markdown.nodes import MarkdownNode


def handle_page_context_html(app, pagename_, templatename_, context, doctree):
    """Makes search compatible with the contents of Markdown documents

    To enable, set markdown_search in conf.py
    """
    if not app.config.markdown_search:
        return

    # Drop sourcename so that the default processor does not read from it
    # Ensure that full markdown processed text is written to the sourcename
    # so that the searchindex can read from it
    if not context.get('sourcename'):
        return
    section = doctree.first_child_matching_class(nodes.section)
    if section is None:
        return
    section = doctree[section]
    md_ingest = section.first_child_matching_class(MarkdownNode)
    if md_ingest is None:
        return
    sourcename = context['sourcename']
    context['sourcename'] = ''
    md_ingest = section[md_ingest]
    outname = os.path.join(app.builder.outdir, '_sources', sourcename)
    try:
        os.makedirs(os.path.dirname(outname))
    except OSError:
        pass
    nodetext = md_ingest.astext()
    # Strip out HTML tags for plain text contents
    nodetext = re.sub(r'(?is)<style.*?</style>', '', nodetext)
    nodetext = re.sub(r'(?is)<script.*?</script>', '', nodetext)
    nodetext = re.sub(r'<[^<]+?>', '', nodetext)
    nodetext = re.sub(r'&[^ ;]+?;', '', nodetext)
    encoding = context.get('encoding', 'utf-8')
    with open(outname, 'w') as writer:
        writer.write(nodetext.encode(encoding, 'ignore'))
