#!/bin/bash
#
# IOFS EA Test cases

echo "------IOFS EA Test cases--------"


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
 yum -y remove opae*
 yum -y install opae-*
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

found=$(lspci | grep 0b2b)
echo $found
if [ "$1" != "" ]; then
  bus_num=$1
  if [[ $found != *"$bus_num"* ]]; then
      echo "Invalid bus number param: "$bus_num
      exit 1
  fi
else
  bus_num=${found:0:2}
  if [[ $((16#$bus_num)) -lt 0 ]]; then
      echo "Invalid bus number: "$bus_num
      exit 1
  fi
fi

gbs_file=nlb_mode_0_unsigned.gbs
iofs_file=ofs_fim_page1_unsigned.bin
echo "FPGA PCIE bus number:" $bus_num
echo "FPGA GBS file:" $gbs_file
echo "FPGA IOFS FIM:" iofs_file

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
"hello_events -B 0x$bus_num"
"packager gbs-info --gbs $gbs_file"
"fpgametrics -B 0x$bus_num"
"bitstreaminfo -v  $iofs_file"
"(echo Y && echo Y) |sudo -u lab PACSign SR -t update -H openssl_manager  -i ofs_fim_page1_unsigned.bin  -o test_fim.bin -r darby_dev_fim_root_public_256.pem  -k darby_dev_fim_csk0_public_256.pem"
"echo yes | fpgasupdate   darby_dev_fim_root_hash.bin"
"fpgainfo security -B 0x$bus_num"
"echo yes | fpgasupdate  darby_dev_bmc_root_hash.bin"
"fpgainfo security -B 0x$bus_num"
"hello_fpga -B 0x$bus_num"
"fpgaconf -B 0x$bus_num -V gbs_file"

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
