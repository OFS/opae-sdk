#!/bin/bash
set -x
PYTHON_VERSION=$(python3 --version | cut -d ' ' -f2)

#install requirements
python3 -m pip install --user -r doc/sphinx/requirements.txt
if [ $? != 0 ];
then
	echo "failed to install Python requirements"
	exit 1
fi
if [ -d mybuild_docs ];
then
	rm -rf mybuild_docs
fi
mkdir mybuild_docs
pushd mybuild_docs
export PYTHONPATH=$PWD/lib/python$PYTHON_VERSION
export PATH=~/.local/bin:$PATH
trap "popd" EXIT

cmake .. -DOPAE_BUILD_SPHINX_DOC=ON -DOPAE_PYTHON_VERSION=$PYTHON_VERSION
make _opae -j $(nproc)
make docs
make manpages

if test $? -eq 0
then
    echo "Sphinx documentation build FINISHED"
else
    echo "Spinx documentation build FAILED"
    exit 1
fi
