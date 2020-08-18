#!/bin/bash

_=${OPAE_SDK_ROOT:=..}

declare -i C_CODE_OK=1
declare -i CPP_CODE_OK=1
declare -i PY_CODE_OK=1

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
    clang-format-3.9 -i -style=Google ${FILES}

    if [ x"$(git diff)" != x ]; then
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

check_py () {
    pushd $(dirname $0)/.. >/dev/null

    PYCODESTYLE=$(which pycodestyle)
    PYLINT=$(which pylint)
    FILES=$(find . -iname "*.py" -not -name "test_fpgadiag.py" \
	    -not -name "cpplint.py" \
	    -not -name "setup.py" \
	    -not -name "buildit.py" \
	    -not -path "./doc/*" \
	    -not -path "./tools/extra/packager/jsonschema-2.3.0/*" \
	    -not -path  "./opae-libs/pyopae/pybind11/*" \
	    -not -path "./python/pacsign/*" \
	    -and \( ! -name "__init__.py" \))
    FILES+=" "
    FILES+=$(grep -rl "^#./usr/bin.*python" ./* \
	    | grep -v cpplint.py \
	    | grep -vE "^\.\/(doc|opae-libs\/pyopae\/pybind11)\/")

    if [ "$TRAVIS_COMMIT_RANGE" != "" ]; then
        CHANGED_FILES=$(git diff --name-only $TRAVIS_COMMIT_RANGE)
        FILES=$(comm -12 <(sort <(for f in $FILES; do printf "$f\n"; done)) <(sort <(for f in $CHANGED_FILES; do printf "./$f\n"; done)))
        printf "Looking at changed files:\n${FILES}"
    fi

    if [ "$1" == "-v" ]; then
	    echo "Checking the following files:"
	    echo ${FILES}
    fi

    if [ "${FILES}" == "" ]; then
        PY_CODE_OK=0
        return ${PY_CODE_OK}
    fi

    if [ ! -x "$PYLINT" ]; then
	    echo "no pylint"
	    popd >/dev/null
	    PY_CODE_OK=1
        return ${PY_CODE_OK}
    fi

    if [ ! -x "$PYCODESTYLE" ]; then
	    echo "no pycodestyle"
	    popd >/dev/null
	    PY_CODE_OK=1
        return ${PY_CODE_OK}
    fi

    echo -e "\n===== pycodestyle ====="
    $PYCODESTYLE $FILES --exclude=test_*.py
    if [ $? -ne 0 ]; then
	    echo "test-codingstyle-py FAILED"
	    popd >/dev/null
	    PY_CODE_OK=1
        return ${PY_CODE_OK}
    fi

    echo -e "\n===== pylint -E ====="
    $PYLINT -E -f parseable --disable=E0401 --ignore=__init__.py --ignore-patterns="test_.*.py" $FILES
    if [ $? -ne 0 ]; then
	    echo "test-codingstyle-py FAILED"
	    popd >/dev/null
	    PY_CODE_OK=1
        return ${PY_CODE_OK}
    fi

    echo "test-codingstyle-py PASSED"
    popd >/dev/null
    PY_CODE_OK=0
    return ${PY_CODE_OK}
}

check_c
check_cpp
check_py

declare -i res=0

if [ ${C_CODE_OK} -ne 0 ]; then
    printf "C FAILED.\n"
    res=1
else
    printf "C PASSED.\n"
fi
if [ ${CPP_CODE_OK} -ne 0 ]; then
    printf "C++ FAILED.\n"
    res=1
else
    printf "C++ PASSED.\n"
fi
if [ ${PY_CODE_OK} -ne 0 ]; then
    printf "Python FAILED.\n"
    res=1
else
    printf "Python PASSED.\n"
fi

if [ ${res} -eq 0 ]; then
    printf "All coding style checks PASSED.\n"
fi
exit ${res}
