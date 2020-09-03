#!/bin/bash
#
# IOFS EA Test cases

echo "IOFS EA Test cases"

echo "input $1"
if [ -z "$1" ]; then

    echo "No argument supplied"
    FPGA_BIN_PATH=$PWD
else
   FPGA_BIN_PATH=$1
fi

echo "FPGA_BIN_PATH: $PWD"


 opae_unit_tests()
 {
 echo "--opae_unit_tests--"
 rm -rf opae-sdk
 git clone https://github.com/OPAE/opae-sdk.git
 cd opae-sdk
 mkdir mybuild
 cd mybuild
  echo "--START HW UNIT TESTS--"
 cmake ..   -DCMAKE_BUILD_TYPE=Debug -DOPAE_BUILD_TESTS=ON -DOPAE_ENABLE_MOCK=OFF
 make -j
 OPAE_EXPLICIT_INITIALIZE=1 LD_LIBRARY_PATH=$PWD/lib make test
 echo "--END HW UNIT TESTS--"

 echo "--START CREATE RPM--"
 rm -rf *
 cmake ..   -DCPACK_GENERATOR=RPM -DOPAE_BUILD_LEGACY=ON -DOPAE_BUILD_SIM=ON
 make -j
 make package_rpm

pkg="opae"
if rpm -q $pkg; then
  echo "$pkg installed"
  yum -y remove $(rpm -qa | grep opae)
else
  echo "$pkg NOT installed"
fi
 cd ../..
 echo "--END CREATE RPM--"

}
 #opae_unit_tests

found=$(ls /dev/dfl-* | wc -l)
echo $found
if [[ $found -lt 1 ]]; then
    echo "Failed to locate drivers."
    exit 1
fi

# BBS & GBS files
PAC_D5005=0b2b
device="/dev/dfl-fme.0"


nlb_mode_0_unsigned_gbs=nlb_mode_0_unsigned.gbs
nlb_mode_3_unsigned_gbs=nlb_mode_3_unsigned.gbs
dummy_afu_pim_unsigned_gbs=dummy_afu_pim_unsigned.gbs
hello_afu_unsigned_gbs=hello_afu_unsigned.gbs


nlb_mode_0_gbs=nlb_mode_0.gbs
nlb_mode_3_gbs=nlb_mode_3.gbs
dummy_afu_pim_gbs=dummy_afu_pim.gbs
hello_afu_gbs=hello_afu.gbs
max10_fw=darby_rot_max10_v2.0.6_nios_fw_v2.0.12_signed.bin
iofs_file=ofs_fim_page1_unsigned.bin

darby_dev_bmc_root_hash=darby_dev_bmc_root_hash.bin
darby_dev_fim_root_hash=darby_dev_fim_root_hash.bin


echo "----BBS & GBS files Start----"
echo "FPGA PCIE bus number        :" $bus_num
echo "nlb_mode_0_unsigned_gbs     :" $nlb_mode_0_unsigned_gbs
echo "nlb_mode_3_unsigned_gbs     :" $nlb_mode_3_unsigned_gbs
echo "dummy_afu_pim_unsigned      :" $dummy_afu_pim_unsigned_gbs
echo "hello_afu_unsigned_gbs      :" $hello_afu_unsigned_gbs
echo "nlb_mode_0_gbs              :" $nlb_mode_0_gbs
echo "nlb_mode_3_gbs              :" $nlb_mode_3_gbs
echo "dummy_afu_pim_gbs           :" $dummy_afu_pim_gbs
echo "hello_afu                   :" $hello_afu_gbs
echo "max10_fw                    :" $max10_fw
echo "FPGA IOFS FIM               :" $iofs_file

echo "darby_dev_bmc_root_hash     :" $darby_dev_bmc_root_hash
echo "darby_dev_fim_root_hash     :" $darby_dev_fim_root_hash

echo "----BBS & GBS files End----"

found=$(lspci -D | grep $PAC_D5005)
echo "PCIe fpga: $found"
found_count=$(lspci -D | grep 0b2b | wc -l)
echo "PCIe found_count: $found_count"

if [ "$found_count" -gt 0 ]; then
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

echo "bus_num: $bus_num"
echo "bdf: $bdf"
echo "sbdf: $sbdf"

echo "bus_num: $bus_num"
echo "bdf: $bdf"
echo "sbdf: $sbdf"

# Set huge pages
echo "echo 20 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages"

sudo sh -c "echo 20 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages"



#FPGA port assign Relesse test
 iofs_fpgaport_attach_release()
 {
  echo "sriov total vfs:"
  sh -c "cat  /sys/bus/pci/devices/$sbdf/sriov_totalvfs"

  echo "*** Release the port controlled by the PF ***"
  fpgaport release $device 0

  fpgainfo port -B 0x$bus_num

  sudo sh -c "echo 1 > /sys/bus/pci/devices/$sbdf/sriov_numvfs"

  lspci | grep acc

  echo "*** Assign the port controlled by the PF ***"

  sudo sh -c "echo 0 > /sys/bus/pci/devices/$sbdf/sriov_numvfs"

  sudo fpgaport assign $device 0

  fpgainfo port -B 0x$bus_num

  lspci | grep acc
 }




tool_tests=(
"lspci | grep acc"
"ls /dev/dfl-fme*"
"ls /dev/dfl-port*"
"fpgainfo fme -B 0x$bus_num"
"fpgainfo port -B 0x$bus_num"
"fpgainfo mac -B 0x$bus_num"
"fpgainfo phy -B 0x$bus_num"
"fpgainfo temp -B 0x$bus_num"
"fpgainfo bmc -B 0x$bus_num"
"fpgainfo power -B 0x$bus_num"
"fpgainfo errors all -B 0x$bus_num"
"fpgainfo security -B 0x$bus_num"
"fpgaconf -B 0x$bus_num -V $FPGA_BIN_PATH/$nlb_mode_3_unsigned_gbs"
"fpgainfo port -B 0x$bus_num"
"fpgadiag -t fpga -m read --read-vc=vh0 --write-vc=vh0 --multi-cl=4 --begin=1024 --end=1024 --timeout-sec=5"
"fpgadiag -t fpga -m write --read-vc=vh0 --write-vc=vh0 --multi-cl=4 --begin=1024 --end=1024 --timeout-sec=5"
"fpgadiag -t fpga -m trput --read-vc=vh0 --write-vc=vh0 --multi-cl=4 --begin=1024 --end=1024 --timeout-sec=5"
"perf list | grep dfl"
"sensors"
"hello_events -B 0x$bus_num"
"packager gbs-info --gbs $FPGA_BIN_PATH/$nlb_mode_0_unsigned_gbs"
"fpgametrics -B 0x$bus_num"
"bitstreaminfo -v  $iofs_file"
"(echo Y && echo Y) |sudo -u lab PACSign SR -t update -H openssl_manager  -i ofs_fim_page1_unsigned.bin  -o test_fim.bin -r darby_dev_fim_root_public_256.pem  -k darby_dev_fim_csk0_public_256.pem"
"fpgaconf -B 0x$bus_num -V $FPGA_BIN_PATH/$nlb_mode_0_unsigned_gbs"
"fpgainfo port -B 0x$bus_num"
"hello_fpga -B 0x$bus_num"
"fpgaconf -B 0x$bus_num -V $FPGA_BIN_PATH/$dummy_afu_pim_unsigned_gbs"
"fpgainfo port -B 0x$bus_num"
"dummy_afu mmio"
"dummy_afu ddr"
"dummy_afu lpbk"
"bitstreaminfo -v  $FPGA_BIN_PATH/$nlb_mode_0_gbs"
"bitstreaminfo -v  $FPGA_BIN_PATH/$nlb_mode_0_unsigned_gbs"
"userclk -B 0x$bus_num -L 200 -H 400"
"bitstreaminfo -v $max10_fw"
"iofs_fpgaport_attach_release"

# fpgasudpate tests
"(echo yes && echo yes)| fpgasupdate  $FPGA_BIN_PATH/darby_dev_bmc_root_hash.bin $bdf"
"fpgainfo security -B 0x$bus_num"

"(echo yes && echo yes)| fpgasupdate  $FPGA_BIN_PATH/darby_dev_fim_root_hash.bin $bdf"
"fpgainfo security -B 0x$bus_num"

)

num_tests_passed=0
num_tests_failed=0
declare -a failed_tests=()


for tool_test in "${tool_tests[@]}"; do
  echo -e "Executing Test: \"${tool_test}\" \n"
  eval output=\`$tool_test\`
  ec=$?
  echo "$output"
  echo -e "\nExit Code: $ec \n \n"

  if [[ $ec == 0  && $parse_ec == 0 ]]; then
    num_tests_passed=$((num_tests_passed + 1))
  else
    num_tests_failed=$((num_tests_failed + 1))
    failed_tests+=("$tool_test")
  fi
done
