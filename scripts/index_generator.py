#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

"""

"""
import os
from jinja2 import Environment

index_tpl = """
<html>
<title>{% block title %}Open Programmable Acceleration Engine{% endblock %}
</title>
<body bgcolor='#85C1E9'>
<h1>Open Programmable Acceleration Engine</h1>
<ul>
{% for ver in versions %}
  <li><a href="{{ ver }}/index.html">{{ ver }}</a></li>
{% endfor %}
</ul>
</body>
</html>"""


def main():
    versions = []
    for x in os.listdir('.'):
        if os.path.isdir(x):
            if x != ".git":
                versions.append(x)
    versions.sort()
    t = Environment().from_string(index_tpl)
    f = open('index.html', 'w')
    f.write(t.render(versions=versions))
    f.close()


if __name__ == "__main__":
    main()
