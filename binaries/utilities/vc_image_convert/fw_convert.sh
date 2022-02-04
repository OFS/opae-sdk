#!/bin/bash
OUTPUT_PATH=./output_files
if [ -n "$1" ]
then
	src_file=$1
else
	read -p "Input firmware file path: " src_file
fi
dest_file=`echo ${src_file##*/} | sed "s/bin/ihex/"`
echo -e "convert $src_file to $dest_file"
objcopy --input-target=binary --output-target=ihex $src_file $OUTPUT_PATH/$dest_file
