#!/bin/bash


pip install --user -r doc/sphinx/requirements.txt

mkdir mybuild_docs
pushd mybuild_docs

trap "popd" EXIT

cmake .. -DBUILD_SPHINX_DOC=ON
make

echo "build Sphinx documentation FINISHED"


COMMIT_ID="$(git rev-parse HEAD)"
TEMP=$((git describe --exact-match $COMMIT_ID) 2>&1)

if [[ $TEMP != *"fatal"* ]]; then
    echo "New tag detected; Uploading documentation under new tag on opae.github.io"
    /bin/bash ../scripts/push-documentation.sh new_tag
else
    echo "No new tag detected; Latest documentation will be uploaded to opae.github.io"
    /bin/bash ../scripts/push-documentation.sh latest
fi    
