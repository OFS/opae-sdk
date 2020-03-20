#!/bin/bash

COMMIT_ID="$(git rev-parse HEAD)"
TEMP=$((git describe --exact-match $COMMIT_ID) 2>&1)
export PYTHONPATH=$PWD/lib/python2.7:$PWD/lib/python3.6

if [[ $TEMP == *"fatal"* ]]; then
    echo "No new tag detected, hence builiding latest docs from master"
    git reset --hard origin/master
fi

#install requirements
pip install --user -r doc/sphinx/requirements.txt
if [ -d mybuild_docs ];
then
	rm -rf mybuild_docs
fi

mkdir mybuild_docs
pushd mybuild_docs

trap "popd" EXIT

cmake .. -DOPAE_BUILD_SPHINX_DOC=ON
make docs
make manpages

if test $? -eq 0
then
    echo "Sphinx documentation build FINISHED"
else
    echo "Spinx documentation build FAILED"
    exit 1
fi



if [[ $TEMP != *"fatal"* ]]; then
    echo "New tag detected; Uploading documentation under new tag on opae.github.io"
    /bin/bash ../scripts/push-documentation.sh new_tag
else
    echo "No new tag detected; Latest documentation will be uploaded to opae.github.io"
    /bin/bash ../scripts/push-documentation.sh latest
fi
