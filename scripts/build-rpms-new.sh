#!/bin/bash
set -x

[ $# -gt 1 ] && src=$(realpath ${1:-$PWD})
[ $# -gt 2 ] && src=$(realpath ${2:-$PWD})

cmake=cmake
if ! command -v cmake > /dev/null && command -v cmake3 > /dev/null; then
  cmake=cmake3
fi

${src}/packaging/opae/rpm/create fedora
