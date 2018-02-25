#!/bin/bash

pip install --user -r doc/sphinx/requirements.txt

mkdir mybuild_docs
pushd mybuild_docs

trap "popd" EXIT

cmake .. -DBUILD_SPHINX_DOC=ON
make
make docs

echo "build Sphinx documentation FINISHED"
