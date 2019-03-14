#!/bin/bash
OUTPUT_PATH=./output_files
SOF_FILE=./input_files/chip.sof
COF_FILE=vcp_gen_pof.cof
DTB_FILE=./input_files/vc_dtb.bin

cof_file=temp.cof
while read line
do
	pof_file=`echo $line | sed -n '/<output_filename>/p' | sed -n 's/<output_filename>//gp' | sed -n 's/<\/output_filename>//gp'`
	if [ -n "$pof_file" ]
	then
		break
	fi
done < $COF_FILE
ext=`date +%Y-%m-%d_%H:%M:%S`.bin
dest_file=${pof_file##*/}
bin_file=${dest_file%.*}'.bin'
dest_reversed_file=vc_reverse.bin
dest_reversed_lite_file=vc_reverse_flash.bin
dest_file=${dest_file%.*}'_'$ext

while getopts f:u:d:o:h opt
do
	case "$opt" in
	f) factory_sof=$OPTARG;;
	u) user_sof=$OPTARG;;
	d) dtb_bin=$OPTARG;;
	o) dest_file=$OPTARG;;
	h) echo -e "Usage: $0 -u [sof_file] -o [bin_file]\n\t-f\tSet factory image file\n\t-u\tSet user image file\n\t-d\tSet device tree table file name\n\t-o\tSet output file name";exit 0;;
	*) exit 0;;
	esac
done

if [ ! -n "$user_sof" ]
then
	user_sof=$SOF_FILE
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
if [ ! -n "$dtb_bin" ]
then
	dtb_bin=$DTB_FILE
fi

sed "s!factory_image.sof!$factory_sof!" $COF_FILE > $cof_file
sed -i "s#user_image.sof#$user_sof#" $cof_file

echo "Step1: convert factory image ${factory_sof##*/} and user image ${user_sof##*/} to ${pof_file##*/}"
quartus_cpf -c $cof_file

echo "Step2: convert $pof_file to ihex"
quartus_cpf -c $pof_file $OUTPUT_PATH/temp.hexout

echo "Step3: convert ihex to bin"
nios2-elf-objcopy -I ihex -O binary $OUTPUT_PATH/temp.hexout $OUTPUT_PATH/$bin_file

echo "Step4: merge device table"
python reverse.py $dtb_bin ./max10_device_table.bin
python merge_device_table.py $OUTPUT_PATH/$bin_file

echo "Step5: generate target bin"
python extract_fpga_image.py $OUTPUT_PATH/$bin_file $OUTPUT_PATH/$dest_file

echo "Step6: generate reversed target bin for factory pre-programming"
python reverse.py $OUTPUT_PATH/$bin_file $OUTPUT_PATH/$dest_reversed_file
python reverse.py $OUTPUT_PATH/$dest_file $OUTPUT_PATH/$dest_reversed_lite_file
rm $OUTPUT_PATH/temp.*
rm $cof_file
echo "========================================================================="
echo "$dest_file for fpgaflash generated successfully"
echo "$dest_reversed_file and $dest_reversed_lite_file for factory pre-programming generated successfully"
