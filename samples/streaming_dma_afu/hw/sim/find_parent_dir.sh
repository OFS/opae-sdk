#!/bin/bash

find_dir() {
    local dir="${1}"
    while [[ -n "${dir}" ]]; do
        [[ -e "${dir}/${2}" ]] && {
            echo "${dir}"
            return
        }
        dir="${dir%/*}"
    done
    [[ -e /"$1" ]] && echo /
}

if [ "${1}" == '' ]; then
    echo >&2 "Usage: find_parent_dir.sh <dirname>"
    exit 1
fi

#get exact script path
SCRIPT_PATH=`readlink -f ${BASH_SOURCE[0]}`
#get director of script path
SCRIPT_DIR_PATH="$(dirname $SCRIPT_PATH)"

found_path=`find_dir "${SCRIPT_DIR_PATH}" "${1}"`
if [ -d "${found_path}/${1}" ]; then
    echo "$found_path/${1}"
else
    echo >&2 "Failed to find parent directory named ${1}"
    exit 1
fi
