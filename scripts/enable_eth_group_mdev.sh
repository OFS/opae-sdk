#!/bin/bash

#N3000 Eth group mdev
set -m

pcie_count=$(ls /sys/bus/pci/devices/*/device | wc -l)
if [[ $pcie_count -lt 1 ]]; then
    echo "Failed to locate drivers."
    exit 1
fi
echo $pcie_count
#Bus number
bus_num=$1
PCIE_DEVICES=/sys/bus/pci/devices/*$bus_num*/device
echo $PCIE_DEVICES
for file in $PCIE_DEVICES
do
    echo "Device $file"
    device_id=$(cat $file)
    echo $device_id
	if [[ $device_id != 0x0b30 ]]; then
		echo "Failed to locate drivers."
		exit 1
	fi

done

echo " found fpga device $file"

FPGA_FEATURID=/sys/bus/pci/devices/*$bus_num*/fpga_region/region*/dfl-fme*/dfl-fme*/feature_id
echo " Feature: $FPGA_FEATURID"
feature_list=()
for file in $FPGA_FEATURID
do
    echo "Feaure id $file"
    feature_id=$(cat $file)
    echo "Feature id: $feature_id"
	if [[ $feature_id != 0x10 ]]; then
       continue
	fi
	feature=$(echo "$file" | cut -d '/' -f 10)
	echo "feature: $feature"
	feature_list+=("$feature")
	
done


modprobe vfio_pci
modprobe vfio_iommu_type1
modprobe vfio_mdev
modprobe vfio_mdev_dfl
var=0
gudi_array=("83b8f4f2-509f-382f-3c1e-e6bfe0fa1001" "83b8f4f2-509f-382f-3c1e-e6bfe0fa1002" "83b8f4f2-509f-382f-3c1e-e6bfe0fa1003")
for name in ${feature_list[@]};
do
    echo "------eth group features------:$name"
	echo "cmd: $name > /sys/bus/dfl/drivers/dfl-eth-group/unbind"
	sudo bash -c "echo $name > /sys/bus/dfl/drivers/dfl-eth-group/unbind"

	echo "cmd: vfio-mdev-dfl > /sys/bus/dfl/devices/$name/driver_override"
	sudo bash -c "echo vfio-mdev-dfl > /sys/bus/dfl/devices/$name/driver_override"

	echo "cmd: echo $name >/sys/bus/dfl/drivers_probe"
	sudo bash -c "echo $name > /sys/bus/dfl/drivers_probe"

	echo "gudi_array: ${gudi_array[var]} "
	echo "echo  ${gudi_array[var]} > /sys/bus/dfl/devices/$name/mdev_supported_types/vfio-mdev-dfl-1/create"
	sudo bash -c "echo  ${gudi_array[var]} > /sys/bus/dfl/devices/$name/mdev_supported_types/vfio-mdev-dfl-1/create"
	var=$((var + 1))
	
done
