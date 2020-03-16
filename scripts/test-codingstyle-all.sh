#!/bin/bash
PATH=~/.local/bin:$PATH

# Function defintion to check cpp coding style
check_c () {
    pushd $(dirname $0)

    CHECKPATCH=checkpatch.pl
    FILES=$(find ../libopae ../common/include/opae ../ase/api/src ../ase/sw ../tools/base/fpgaconf ../tools/base/fpgad \( -iname "*.c" -or -iname "*.h" \) -and \( ! -name "srv.c" \) -and \( ! -path "../libopae/plugins/xfpga/usrclk/*" \) -and \( ! -path "../common/include/opae/cxx/*" \) )

    if [ ! -f $CHECKPATCH ]; then
        wget --no-check-certificate https://raw.githubusercontent.com/torvalds/linux/master/scripts/checkpatch.pl
        if [ ! -f $CHECKPATCH ]; then
            echo "Couldn't download checkpatch.pl - please put a copy into the same"
            echo "directory as this script."
            popd
            C_CODE_OK=1
            echo "test-codingstyle-c FAILED"
            return "$C_CODE_OK"
        fi
    fi

    perl ./$CHECKPATCH --no-tree --no-signoff --terse -f $FILES | grep -v "need consistent spacing" | grep ERROR



    if [ $? -eq 0 ]; then
        echo "test-codingstyle-c FAILED"
        popd
        C_CODE_OK=1
    else
        echo "test-codingstyle-c PASSED"
        popd
        C_CODE_OK=0
    fi
    return "$C_CODE_OK"
}

# Function defintion to check cpp coding style
check_cpp () {
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
        CPP_CODE_OK=1
    else
        echo "test-codingstyle-cpp PASSED"
        popd
        CPP_CODE_OK=0
    fi
    return "$CPP_CODE_OK"
}

# Function definiton to check python coding style
check_py () {
    pushd $(dirname $0)/..

    PYCODESTYLE=$(which pycodestyle)
    PYLINT=$(which pylint)
    FILES=$(find . -iname "*.py" -not -name "test_fpgadiag.py" -not -name "cpplint.py" -not -name "setup.py" -not -path "./doc/*" -not -path "./tools/extra/packager/jsonschema-2.3.0/*" -not -path  "./pyopae/pybind11/*" -and \( ! -name "__init__.py" \))
    FILES+=" "
    FILES+=$(grep -rl "^#./usr/bin.*python" ./* | grep -v cpplint.py | grep -vE "^\.\/(doc|pyopae\/pybind11)\/")

    if [ "$CI_COMMIT_BEFORE_SHA" != "" ]; then
        CHANGED_FILES=$(git diff --name-only $CI_COMMIT_BEFORE_SHA)
        printf "Looking at changed files:\n${CHANGED_FILES}"
        FILES=$(comm -12 <(sort <(for f in $FILES; do printf "$f\n"; done)) <(sort <(for f in $CHANGED_FILES; do printf "./$f\n"; done)))
    fi

    if [ "$1" == "-v" ]; then
	    echo "Checking the following files:"
	    echo $FILES
    fi

    if [ "$FILES" == "" ]; then
        PY_CODE_OK=0
        return "$PY_CODE_OK"
    fi

    if [ ! -x "$PYLINT" ]; then
	    echo "no pylint"
	    popd
	    PY_CODE_OK=1
        return "$PY_CODE_OK"
    fi

    if [ ! -x "$PYCODESTYLE" ]; then
	    echo "no pycodestyle"
	    popd
	    PY_CODE_OK=1
        return "$PY_CODE_OK"
    fi

    echo -e "\n===== pycodestyle ====="
    $PYCODESTYLE $FILES --exclude=test_*.py
    if [ $? -ne 0 ]; then
	    echo "test-codingstyle-py FAILED"
	    popd
	    PY_CODE_OK=1
        return "$PY_CODE_OK"
    fi

    echo -e "\n===== pylint -E ====="
    $PYLINT -E -f parseable --ignore=__init__.py --ignore-patterns="test_.*.py" $FILES
    if [ $? -ne 0 ]; then
	    echo "test-codingstyle-py FAILED"
	    popd
	    PY_CODE_OK=1
        return "$PY_CODE_OK"
    fi

    echo "test-codingstyle-py PASSED"
    popd
    PY_CODE_OK=0
    return "$PY_CODE_OK"
}

# Function call to check cpp coding style
check_c
check_cpp
check_py


if [ "$PY_CODE_OK" == "1" ] || [ "$C_CODE_OK" == "1" ] || [ "$CPP_CODE_OK" == "1" ]; then
    echo "Coding style check failed. Please fix the issues described above."
    exit 1
else
    echo "Coding style check passed."
    exit 0
fi
