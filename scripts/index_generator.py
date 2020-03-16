#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

"""

"""
from __future__ import absolute_import
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
  transform: translate(-50%, 0%)" align="left">

<h1>Open Programmable Acceleration Engine - Documentation</h1>

<hr/>
<p>The Open Programmable Acceleration Engine is a software framework for
managing and accessing programmable accelerators (FPGAs). Its main parts
are:</p>

<ul>
    <li> - The OPAE Software Development Kit (OPAE SDK),</li>
    <li> - The OPAE Linux driver for Intel(R) Xeon(R) CPU with Integrated
    FPGAs and Intel(R) PAC with Arria(R) 10 GX FPGA</li>
    <li> - The Basic Building Block (BBB) library for accelerating AFU
    development (not part of the OPAE release, but pre-release code is
    available on GitHub:
    <a href="https://github.com/OPAE/intel-fpga-bbb">Intel FPGA BBB</a></li>
    </ul>
<p> </p>
<p>OPAE is under active development to extend to more hardware platforms,
 as well as to build up the software stack with additional abstractions
 to enable more software developers.</p>

<p>
    The OPAE SDK is a collection of libraries and tools to facilitate
     the development of software applications and accelerators using
     OPAE.
    It provides a library implementing the OPAE C API for presenting
    a streamlined and easy-to-use interface for software applications
    to discover, access, and manage FPGA devices and accelerators using
     the OPAE software stack. The OPAE SDK also includes the AFU Simulation
      Environment (ASE) for end-to-end simulation of accelerator RTL together
       with software applications using the OPAE C API.
    OPAE's goal is to accelerate FPGA adoption. It is a community effort
     to simplify the development and deployment of FPGA applications, so
     we explicitly welcome discussions and contributions! The OPAE SDK
      source, unless otherwise noted, is released under a BSD 3-clause
      license.
</p>

<p>More information about OPAE can be found at
<a href="http://01.org/OPAE">http://01.org/OPAE.</a></p>
<hr/>
<h3>Please select one of the links below to browse the online OPAE
documentation:</h3>

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
            return list(map(int, s.split('.')))
        except ValueError:
            return s
    versions.sort(key=splitit)
    t = Environment().from_string(index_tpl)
    f = open('index.html', 'w')
    f.write(t.render(versions=versions))
    f.close()


if __name__ == "__main__":
    main()
