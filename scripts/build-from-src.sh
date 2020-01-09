#!/bin/bash

cd source_code

tar -xvzf ./opae-*.tar.gz
cd opae-*/usr

if [ -d "local" ]
then
  cd local
fi

rm -rf build
mkdir -p build
cd build

cmake ..

make > >(tee -a output_stdout.txt) 2> >(tee -a output_stderr.txt >&2)

output_stdout=$(cat output_stdout.txt)
output_stderr=$(cat output_stderr.txt)
WARNING="Warnings detected in build output."

if [[ "$output_stdout" == *"warning:"* ]] || [[ "$output_stderr" == *"warning:"* ]]
then
  echo $WARNING
fi
