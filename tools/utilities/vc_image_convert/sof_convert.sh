#!/bin/bash
QUARTUS_ROOT=/home/sse-cd-1/intelFPGA_pro/18.0
OUTPUT_PATH=./output_files
COF_FILE=vcp_gen_pof.cof

if [ $# == 0 ]
then
	echo "Usage: $0 -u [sof_file] -o [bin_file]"
	exit 0;
fi

cof_file=temp.cof
pof_file=`. ./get_pof_name_from_cof.sh $COF_FILE`
ext=`date +%Y-%m-%d_%H:%M:%S`.bin
dest_file=${pof_file##*/}
dest_file=${dest_file%.*}'_'$ext

while getopts f:u:o:c:h opt
do
	case "$opt" in
	f) factory_sof=$OPTARG;;
	u) user_sof=$OPTARG;;
	o) dest_file=$OPTARG;;
	h) echo -e "Usage: $0 -u [sof_file] -o [bin_file]\n\t-f\tSet factory image file\n\t-u\tSet user image file\n\t-o\tSet output file name";exit 0;;
	*) exit 0;;
	esac
done

if [ ! -n "$user_sof" ]
then
	read -p "Input user sof file path: " user_sof
fi
if [ ! -f "$user_sof" ]
then
	echo $user_sof "not found"
	exit 0
fi
if [ ! -n "$factory_sof" ]
then
	factory_sof=$user_sof
fi
if [ ! -f "$factory_sof" ]
then
	echo $factory_sof "not found"
	exit 0
fi

sed "s!factory_image.sof!$factory_sof!" $COF_FILE > $cof_file
sed -i "s#user_image.sof#$user_sof#" $cof_file

echo "Step1: convert factory image ${factory_sof##*/} and user image ${user_sof##*/} to ${pof_file##*/}"
$QUARTUS_ROOT/quartus/bin/quartus_cpf -c $cof_file
echo "Step2: convert $pof_file to ihex"
$QUARTUS_ROOT/quartus/bin/quartus_cpf -c $pof_file $OUTPUT_PATH/temp.hexout
echo "Step3: convert ihex to bin"
$QUARTUS_ROOT/nios2eds/bin/gnu/H-x86_64-pc-linux-gnu/bin/nios2-elf-objcopy -I ihex -O binary $OUTPUT_PATH/temp.hexout $OUTPUT_PATH/temp.bin
echo "Step4: generate target bin"
python extract_fpga_image.py $OUTPUT_PATH/temp.bin $OUTPUT_PATH/$dest_file
rm $OUTPUT_PATH/temp.*
rm $cof_file
echo "$dest_file generated successfully"
