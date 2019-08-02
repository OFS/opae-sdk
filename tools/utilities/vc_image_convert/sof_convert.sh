#!/bin/bash
OUTPUT_PATH=./output_files
FACTORY_SOF_FILE=./input_files/vc_factory.sof
USER_SOF_FILE=./input_files/vc_user.sof
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
bin_file=vc.bin
dest_file=vc_flash.bin
dest_reversed_file=vc_reverse.bin
dest_reversed_lite_file=vc_reverse_flash.bin
generate_reverse_bin=0

while getopts f:u:d:o:vrh opt
do
	case "$opt" in
	f) factory_sof=$OPTARG;;
	u) user_sof=$OPTARG;;
	d) dtb_bin=$OPTARG;;
	o) dest_file=$OPTARG;;
	h) echo -e "Usage: $0 -u [sof_file] -o [bin_file]\n\t-f\tSet factory image file\n\t-u\tSet user image file\n\t-d\tSet device tree table file name\n\t-o\tSet output file name\n\t-v\tShow publish date of this tool\n\t-r\tGenerate reversed bin file";exit 0;;
	v) echo "2019.07.23";exit 0;;
	r) generate_reverse_bin=1;;
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
if [ ! -n "$factory_sof" ]
then
	factory_sof=$FACTORY_SOF_FILE
fi
if [ ! -f "$factory_sof" ]
then
	echo $factory_sof "not found, please try -f option to select one"
	exit 0
fi
if [ ! -n "$dtb_bin" ]
then
	dtb_bin=$DTB_FILE
fi
if [ ! -f "$dtb_bin" ]
then
	echo $dtb_bin "not found, please try -d option to select one"
	exit 0
fi

if [ ! -d "$OUTPUT_PATH" ]
then
	mkdir -p $OUTPUT_PATH
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

if [ $generate_reverse_bin == 1 ]
then
    echo "Step6: generate reversed target bin for factory pre-programming"
    python reverse.py $OUTPUT_PATH/$bin_file $OUTPUT_PATH/$dest_reversed_file
fi

echo "========================================================================="
echo "$dest_file for fpgaflash generated successfully"
if [ $generate_reverse_bin == 1 ]
then
    echo "$dest_reversed_file for factory pre-programming generated successfully"
fi
rm $OUTPUT_PATH/temp.*
rm $cof_file
