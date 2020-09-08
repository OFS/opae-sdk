#!/bin/bash

#Create FPGA N3000 Eth group mdev
set -m

FPGA_N3000_ETH_FEATURE_ID=0x10
FPGA_N3000_DEVICE_HEXID=0x0b30
FPGA_N3000_DEVICE_ID=0b30
#Bus number
bus_num=$1



found=$(lspci -D | grep $FPGA_N3000_DEVICE_ID)
echo "PCIe fpga: $found"
fpga_n3000_count=$(lspci -D | grep 0b2b | wc -l)
echo "PCIe found_count:$fpga_n3000_count"

if [ $fpga_n3000_count -eq 0] ; then
 echo -e "Not Found FPGA N3000"
 exit 1
 fi


bus_num=0
# FPGA N3000 device more then one, exit with error
echo "FPGA N3000 device count : $fpga_n3000_count"
if [ "$fpga_n3000_count" -gt 1 ]; then
  echo -e "--- Found $fpga_n3000_count FPGA N3000 device ---"
  read -p "Please enter bus number: " bus_num
  echo "The bus number is : $bus_num"
  if [ -z $bus_num ] ; then
    echo -e "Invalid bus number"
    exit 1
  fi
  device_path=$(realpath /sys/bus/pci/devices/*$bus_num*/device)
  echo $device_path
  device_id=$(cat $device_path)

  if [ $device_id == $FPGA_N3000_DEVICE_HEXID ] ; then
     echo -e "FOUND FPGA N3000 $FPGA_N3000_DEVICE_HEXID\n"
     break
  else
    echo -e "Not Found FPGA N3000"
    exit 1
  fi
fi

if [ "$fpga_n3000_count" -eq 1 ]; then
  bus_num=${found:5:2}
  bdf=${found:5:7}
  sbdf=${found:0:12}
  if [[ $((16#$bus_num)) -lt 0 ]]; then
      echo "Invalid bus number: "$bus_num
      exit 1
  fi
else
   echo "Not Found Intel FPGA"
   exit 1

fi

echo "---Found fpga N3000 device---"
echo "FPGA N3000 bus_num: $bus_num"
echo "FPGA N3000 bdf: $bdf"
echo "FPGA N3000 sbdf: $sbdf"


# Check for eth group feature id
FPGA_ETH_GROUP_PATH=/sys/bus/pci/devices/*$bus_num*/fpga_region/region*/dfl-fme*/dfl-fme*/feature_id
echo " Feature: $FPGA_ETH_GROUP_PATH"
feature_list=()
for file in $FPGA_ETH_GROUP_PATH
do
    echo "Feaure id $file"
    feature_id=$(cat $file)
    echo "Feature id: $feature_id"
	if [[ $feature_id != $FPGA_N3000_ETH_FEATURE_ID ]]; then
       continue
	fi
	feature=$(echo "$file" | cut -d '/' -f 10)
	echo "Feature: $feature"
	feature_list+=("$feature")
	
done

echo "Eth group Feature:${feature_list[*]}"

# load module
modprobe vfio_pci
modprobe vfio_iommu_type1
modprobe vfio_mdev
modprobe vfio_mdev_dfl
var=0
# two guid array, Host guid "83b8f4f2-509f-382f-3c1e-e6bfe0fa1001" 
# Ethernet guid "83b8f4f2-509f-382f-3c1e-e6bfe0fa1002"
guid_array=("83b8f4f2-509f-382f-3c1e-e6bfe0fa1001" "83b8f4f2-509f-382f-3c1e-e6bfe0fa1002")
for name in ${feature_list[@]};
do
    echo "------eth group features------:$name"
	echo "cmd: $name > /sys/bus/dfl/drivers/dfl-eth-group/unbind"
	bash -c "echo $name > /sys/bus/dfl/drivers/dfl-eth-group/unbind"

	echo "cmd: vfio-mdev-dfl > /sys/bus/dfl/devices/$name/driver_override"
	bash -c "echo vfio-mdev-dfl > /sys/bus/dfl/devices/$name/driver_override"

	echo "cmd: echo $name >/sys/bus/dfl/drivers_probe"
	bash -c "echo $name > /sys/bus/dfl/drivers_probe"

	echo "guid_array: ${guid_array[var]} "
	echo "echo  ${guid_array[var]} > /sys/bus/dfl/devices/$name/mdev_supported_types/vfio-mdev-dfl-1/create"
	bash -c "echo  ${guid_array[var]} > /sys/bus/dfl/devices/$name/mdev_supported_types/vfio-mdev-dfl-1/create"
	var=$((var + 1))
	
done

