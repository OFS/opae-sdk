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
<title>{% block title %}Open Programmable Acceleration Engine{% endblock %}</title>
<body>
<ul>
{% for ver in versions %}
  <li><a href="{{ ver }}/index.html">{{ version }}</a></li>
{% endfor %}
</ul>
</body>
</html>"""


def main():
    versions = []
    for x in os.listdir('.'):
        if os.path.isdir(x) :
            if x != ".git": versions.append(x)
    #versions = os.listdir(os.getcwd())
    t = Environment().from_string(index_tpl)
    f = open('index.html', 'w')
    f.write(t.render(versions=versions))
    f.close()

if __name__ == "__main__":
    main()
