#!/bin/bash -e

set -o xtrace

FILES=$(find ./tools/extra/pyfpgadiag)
if [ "$TRAVIS_COMMIT_RANGE" != "" ]; then
    CHANGED_FILES=$(git diff --name-only $TRAVIS_COMMIT_RANGE)
    printf "Looking at changed files:\n${CHANGED_FILES}"
    FILES=$(comm -12 <(sort <(for f in $FILES; do printf "$f\n"; done)) <(sort <(for f in $CHANGED_FILES; do printf "./$f\n"; done)))
fi

echo "$FILES"

if [ "$FILES" == "" ]; then
	echo "No applicable files changed, skipping tests"
	exit 0
fi

mkdir -p pytests
pushd pytests

if [ ! -f CMakeCache.txt ]; then
	cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF -DBUILD_LIBOPAE_PY=ON -DBUILD_PYTHON_DIST=ON
fi

echo "Making pyopae"
make -j4 opae-c opae-cxx-core pyopae-dist

python -m pip install pyopae/stage/dist/opae.fpga*.whl --user
popd
LD_LIBRARY_PATH=$PWD/pytests/lib python -m nose2 -C --coverage-report term-missing test_fpgadiag -s pytests/tools/extra/pyfpgadiag/stage --coverage pytests/tools/extra/pyfpgadiag/stage
pip uninstall opae.fpga -y
