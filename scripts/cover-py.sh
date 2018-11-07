#!/bin/bash -e

set -o xtrace

mkdir -p pytests
cd pytests

if [ ! -f CMakeCache.txt ]; then
	cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF -DBUILD_LIBOPAE_PY=ON -DBUILD_PYTHON_DIST=ON
fi

echo "Making pyopae"
make -j4 opae-c opae-cxx-core pyopae-dist

python -m pip install pyopae/stage/dist/opae.fpga*.whl --user
python -m nose2 -F -C --coverage-report term-missing test_fpgadiag -s tools/extra/pyfpgadiag/stage --coverage tools/extra/pyfpgadiag/stage
pip uninstall opae.fpga
