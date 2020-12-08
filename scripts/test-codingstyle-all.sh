#!/bin/bash

_=${OPAE_SDK_ROOT:=..}

declare -i C_CODE_OK=1
declare -i CPP_CODE_OK=1

find_c() {
    find "${OPAE_SDK_ROOT}/opae-libs/libbitstream" -iname "*.c" -or -iname "*.h"
    find "${OPAE_SDK_ROOT}/opae-libs/libopae-c" -iname "*.c" -or -iname "*.h"
    find "${OPAE_SDK_ROOT}/opae-libs/plugins" \( -iname "*.c" -or -iname "*.h" \) -and \
	    \( ! -path '*/opae-libs/plugins/xfpga/usrclk/*' \)
    find "${OPAE_SDK_ROOT}/samples" -iname "hello_events.c" -or -iname "hello_fpga.c" -or -iname "object_api.c"
    find "${OPAE_SDK_ROOT}/tools/argsfilter" -iname "*.c" -or -iname "*.h"
    find "${OPAE_SDK_ROOT}/tools/extra/ras" -iname "*.c" -or -iname "*.h"
    find "${OPAE_SDK_ROOT}/tools/extra/userclk" -iname "*.c" -or -iname "*.h"
    find "${OPAE_SDK_ROOT}/tools/fpgaconf" -iname "*.c" -or -iname "*.h"
    find "${OPAE_SDK_ROOT}/tools/fpgainfo" -iname "*.c" -or -iname "*.h"
    find "${OPAE_SDK_ROOT}/tools/fpgametrics" -iname "*.c" -or -iname "*.h"
    find "${OPAE_SDK_ROOT}/tools/libboard" -iname "*.c" -or -iname "*.h"
}

check_c () {
    pushd $(dirname $0) >/dev/null

    CHECKPATCH=checkpatch.pl

    if [ ! -f $CHECKPATCH ]; then
        wget --no-check-certificate https://raw.githubusercontent.com/torvalds/linux/master/scripts/checkpatch.pl
        if [ ! -f $CHECKPATCH ]; then
            echo "Couldn't download checkpatch.pl - please put a copy into the same"
            echo "directory as this script."
            popd >/dev/null
            echo "test-codingstyle-c FAILED"
            return ${C_CODE_OK}
        fi
    fi

    FILES=$(find_c)

    perl ./$CHECKPATCH --no-tree --no-signoff --terse -f $FILES | grep -v "need consistent spacing" | grep ERROR

    if [ $? -eq 0 ]; then
        echo "test-codingstyle-c FAILED"
    else
        echo "test-codingstyle-c PASSED"
        C_CODE_OK=0
    fi

    popd >/dev/null

    return ${C_CODE_OK}
}

find_cpp() {
    find "${OPAE_SDK_ROOT}/opae-libs/libopaecxx/src" -type f
    find "${OPAE_SDK_ROOT}/opae-libs/libopaecxx/samples" -type f
    find "${OPAE_SDK_ROOT}/opae-libs/include/opae/cxx/core" -type f
}

check_cpp () {
    pushd $(dirname $0) >/dev/null

    FILES=$(find_cpp)
    echo "Checking $FILES"
    clang-format -i -style=Google ${FILES}

    if [ x"$(git diff -- ${FILES})" != x ]; then
        echo "Coding style check failed. Please fix them based on the following suggestions. You can run clang-format on these files in order to automatically format your code." 
        git --no-pager diff
        echo "The files that need to be fixed are:"
        git diff --name-only
        echo "test-codingstyle-cpp FAILED"
        #git reset --hard HEAD
    else
        echo "test-codingstyle-cpp PASSED"
        CPP_CODE_OK=0
    fi

    popd >/dev/null

    return ${CPP_CODE_OK}
}

if [ "$1" == "c" ]; then
	check_c
	exit ${C_CODE_OK}
elif [ "$1" == "cpp" ]; then
	check_cpp
	exit ${CPP_CODE_OK}
else
	echo "unknown codingstyle check $1"
	exit 1
fi
