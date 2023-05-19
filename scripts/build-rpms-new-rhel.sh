#!/bin/bash
set -x

src=$(realpath ${1:-$PWD})

cmake=cmake
if ! command -v cmake > /dev/null && command -v cmake3 > /dev/null; then
  cmake=cmake3
fi

${src}/packaging/opae/rpm/create rhel
