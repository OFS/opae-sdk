#!/bin/bash
OUTPUT_PATH=./output_files
USER_SOF_FILE=./input_files/chip.sof
COF_FILE=vc_rot.cof

cof_file=temp.cof
while read line
do
	pof_file=`echo $line | sed -n '/<output_filename>/p' | sed -n 's/<output_filename>//gp' | sed -n 's/<\/output_filename>//gp'`
	if [ -n "$pof_file" ]
	then
		break
	fi
done < $COF_FILE
pof_file=`echo $pof_file | sed -n 's/\r//gp'`
dest_file=chip_rot.bin

while getopts u:o:vh opt
do
	case "$opt" in
	u) user_sof=$OPTARG;;
	o) dest_file=$OPTARG;;
	h) echo -e "Usage: $0 -u [sof_file] -o [bin_file]\n\t-u\tSet user image file\n\t-o\tSet output file name\n\t-v\tShow publish date of this tool";exit 0;;
	v) echo "2019.07.31";exit 0;;
	*) exit 0;;
	esac
done

if [ ! -n "$user_sof" ]
then
	user_sof=$USER_SOF_FILE
fi
if [ ! -f "$user_sof" ]
then
	echo $user_sof "not found, please try -u option to select one"
	exit 0
fi

if [ ! -d "$OUTPUT_PATH" ]
then
	mkdir -p $OUTPUT_PATH
fi

sed "s!user_image.sof!$user_sof!" $COF_FILE > $cof_file

echo "Step1: convert user image ${user_sof##*/} to ${pof_file##*/}"
quartus_cpf -c $cof_file

echo "Step2: extract user image"
if [ `command -v python` ]; then
	python extract_rot_image.py $pof_file $OUTPUT_PATH/$dest_file
elif [ `command -v python3` ]; then
	python3 extract_rot_image.py $pof_file $OUTPUT_PATH/$dest_file
fi

rm $cof_file
echo "========================================================================="
echo "$dest_file for digital signing generated successfully"
