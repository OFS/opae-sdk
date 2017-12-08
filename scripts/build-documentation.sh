#!/bin/bash

pip install -r doc/sphinx/requirements.txt

mkdir mybuild_docs
pushd mybuild_docs

trap "popd" EXIT

cmake ..
make sphinx_html

echo "build Sphinx documentation FINISHED"
