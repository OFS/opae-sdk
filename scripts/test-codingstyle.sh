#!/bin/bash

pushd $(dirname $0)

CHECKPATCH=checkpatch.pl
FILES=$(find ../libopae ../common/include/opae ../ase/api/src ../ase/sw ../tools/fpgaconf ../tools/fpgad ../samples \( -iname "*.c" -or -iname "*.h" \) -and \( ! -name "srv.c" \) -and \( ! -path "../libopae/src/usrclk/*" \) -and \( ! -path "../common/include/opae/cxx/*" \) )

if [ ! -f $CHECKPATCH ]; then
	wget --no-check-certificate https://raw.githubusercontent.com/torvalds/linux/master/scripts/checkpatch.pl
	if [ ! -f $CHECKPATCH ]; then
		echo "Couldn't download checkpatch.pl - please put a copy into the same"
		echo "directory as this script."
		popd
		exit 1
	fi
fi

perl ./$CHECKPATCH --no-tree --no-signoff --terse -f $FILES | grep -v "need consistent spacing" | grep ERROR

if [ $? -eq 0 ]; then
	echo "test-codingstyle FAILED"
	popd
	exit 1
else
	echo "test-codingstyle PASSED"
	popd
	exit 0
fi

