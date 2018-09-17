#!/bin/bash

##
## Run latency tests.
##
## Arguments:
##   $1 is the number of channels (default 3).  For DCP use 1.
##   $2 is a tag in the middle of file names.
##

num_channels=3
if [ -n "${1}" ]; then
    num_channels=${1}
fi

tag=""
if [ -n "${2}" ]; then
    tag="${2}_"
fi

mkdir -p stats

for mcl in 1 2 4
do
    for ((vc=0; vc < ${num_channels}; vc++))
    do
        ./test_mem_latency --vcmap-enable=0 --mcl=${mcl} --vc=${vc} | tee stats/lat_${tag}mcl${mcl}_vc${vc}.dat
    done

    if [ $num_channels -gt 1 ]; then
        ./test_mem_latency --vcmap-enable=1 --mcl=${mcl} --vc=0 | tee stats/lat_${tag}map_mcl${mcl}_vc0.dat
    fi
done
