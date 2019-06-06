#!/bin/bash
if [ -n "$1" ]
then
	cof_file=$1
else
	echo -n "Enter .cof file name: "
	read cof_file
fi
cat $cof_file | while read line
do
	echo $line | sed -n '/<output_filename>/p' | sed -n 's/<output_filename>//gp' | sed -n 's/<\/output_filename>//gp'
done
