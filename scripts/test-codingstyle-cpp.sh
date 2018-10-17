#!/bin/bash

pushd $(dirname $0)

FILES=$(find ../libopaecxx/src ../libopaecxx/samples ../common/include/opae/cxx/core \( -iname "*.cpp" -or -iname "*.h" \))
clang-format-3.9 -i -style=Google $FILES

if [ x"$(git diff)" != x ]; then
   echo "Coding style check failed. Please fix them based on the following suggestions. You can run clang-format on these files in order to automatically format your code." 
   git --no-pager diff
   echo "The files that need to be fixed are:"
   git diff --name-only
   echo "test-codingstyle-cpp FAILED"
   git reset --hard HEAD
   popd
   exit 1
else
    echo "test-codingstyle-cpp PASSED"
    popd
fi

