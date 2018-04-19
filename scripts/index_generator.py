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
<head>
<link rel="stylesheet"
 href="https://opae.github.io/latest/_static/css/theme.css"
  type="text/css"/>
<title>Open Programmable Acceleration Engine
</title>
</head>
<body style="background-color: white">
<div class="section" style="background-color: white; padding: 50px;
 position: absolute; left: 50%; margin-right: -50%;
  transform: translate(-50%, 0%) ">

<h1>Open Programmable Acceleration Engine - Documentation</h1>

<hr/>

<p>Please select one of the links below to browse the online OPAE
 documentation:</p>

<ul style="font-size: large; font-weight: bold">

  {% for ver in versions %}
  <li><a href="{{ ver }}/index.html">{{ ver }}</a></li>
{% endfor %}

</ul>
</div>
</body>
</html>"""


def main():
    versions = []
    for x in os.listdir('.'):
        if os.path.isdir(x):
            if x != ".git":
                versions.append(x)

    def splitit(s):
        try:
            return map(int, s.split('.'))
        except ValueError:
            return s
    versions.sort(key=splitit)
    t = Environment().from_string(index_tpl)
    f = open('index.html', 'w')
    f.write(t.render(versions=versions))
    f.close()


if __name__ == "__main__":
    main()
