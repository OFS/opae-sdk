#!/bin/bash -e

MPF_DIR=$1
TARGET_DIR=$2

RELEASE_DIRS="${MPF_DIR}/hw ${MPF_DIR}/sw ${MPF_DIR}/sample ${MPF_DIR}/README_cci_mpf"
EXCLUDE="*.awb"

if [ -e ${TARGET_DIR} ]; then
   echo "'${TARGET_DIR}' already exists, please delete."
   exit 1
fi

cd ${MPF_DIR}/sw
make clean
cd -

cd ${MPF_DIR}/sample/Hello_ALI_VTP_NLB/SW
make clean
cd -

mkdir ${TARGET_DIR}

cp -r ${RELEASE_DIRS} ${TARGET_DIR}
cd ${TARGET_DIR}

for f in ${EXCLUDE}; do
   find . -iname "${f}" -exec rm {} \;
done

cd -


