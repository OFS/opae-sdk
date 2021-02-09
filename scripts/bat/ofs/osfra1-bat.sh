#!/bin/bash
#
# Perform quick BAT test on IOFS R1

echo "************FPGA IOFS R1 BAT***************"

echo -e "FPGA IOFS R1 PF Devices: \n"$(lspci | grep af00)

echo -e "FPGA IOFS R1 VF Devices: \n"$(lspci | grep af01)


found=$(lspci | grep af00)
echo "found:"$found
if [ "$1" != "" ]; then
  bus_num=$1
  echo "----?$1:"$1
  if [[ $found != *"$bus_num"* ]]; then
      echo "Invalid bus number param: "$bus_num
      exit 1
  fi
else
  bus_num=${found:0:2}
  dev_num=${found:3:2}
  fun_num=${found:6:1}
  bdf_num=${found:0:7}
  echo "bus_num:"${found:0:2}
  echo "dev_num:"${found:3:2}
  echo "fun_num:"${found:6:1}
  if [[ $((16#$bus_num)) -lt 0 ]]; then
      echo "Invalid bus number: "$bus_num
      exit 1
  fi
fi

echo "FPGA IOFS R1 PF Bus number:"$bus_num
echo "FPGA IOFS R1 PF dev number:"$dev_num
echo "FPGA IOFS R1 PF fun number:"$fun_num
echo "FPGA IOFS R1 PF bdf number:"$bdf_num



found=$(ls /dev/dfl-* | wc -l)
if [[ $found -lt 1 ]]; then
    echo "Failed to locate drivers."
    exit 1
fi

echo "FPGA Driver is loaded"


tool_tests=(
"fpgainfo errors all -B 0x$bus_num"
"fpgainfo fme -B 0x$bus_num"
"fpgainfo port -B 0x$bus_num"
"fpgainfo mac -B 0x$bus_num"
"fpgainfo phy -B 0x$bus_num"
"fpgainfo temp -B 0x$bus_num"
"fpgainfo bmc -B 0x$bus_num"
"fpgainfo power -B 0x$bus_num"
"fpgainfo security -B 0x$bus_num"

"host_exerciser lpbk"
"host_exerciser --mode lpbk lpbk"
"host_exerciser --mode read lpbk"
"host_exerciser --mode write lpbk"
"host_exerciser --mode trput lpbk"

"host_exerciser --cls cl_1  lpbk"
"host_exerciser --cls cl_2  lpbk"
"host_exerciser --cls cl_3  lpbk"
"host_exerciser --cls cl_4  lpbk"

"host_exerciser --mode lpbk --cls cl_1 lpbk"
"host_exerciser --mode lpbk --cls cl_2 lpbk"
"host_exerciser --mode lpbk --cls cl_3 lpbk"
"host_exerciser --mode lpbk --cls cl_4 lpbk"

"host_exerciser --mode read --cls cl_1 lpbk"
"host_exerciser --mode read --cls cl_2 lpbk"
"host_exerciser --mode read --cls cl_3 lpbk"
"host_exerciser --mode read --cls cl_4 lpbk"

"host_exerciser --mode write --cls cl_1 lpbk"
"host_exerciser --mode write --cls cl_2 lpbk"
"host_exerciser --mode write --cls cl_3 lpbk"
"host_exerciser --mode write --cls cl_4 lpbk"

"host_exerciser --mode trput --cls cl_1 lpbk"
"host_exerciser --mode trput --cls cl_2 lpbk"
"host_exerciser --mode trput --cls cl_3 lpbk"
"host_exerciser --mode trput --cls cl_4 lpbk"

"host_exerciser --mode trput --interleave 0 lpbk"
"host_exerciser --mode trput --interleave 1 lpbk"
"host_exerciser --mode trput --interleave 2 lpbk"

"host_exerciser --mode lpbk mem"
"host_exerciser --mode read mem"
"host_exerciser --mode write mem"
"host_exerciser --mode trput mem"

)

num_tests_passed=0
num_tests_failed=0
declare -a failed_tests=()

for tool_test in "${tool_tests[@]}"; do
  echo "Executing Test: \"${tool_test}\""
  eval output=\`$tool_test\`
  ec=$?
  echo "$output"
  echo "Exit Code: $ec"

  parse_ec=0
  if [[ ${tool_test} == "fpgainfo"* && ${output} == *"0x0B3"* ]]; then
    parse_fpgainfo_output "$tool_test" "${output}"
  fi

  if [[ $ec == 0  && $parse_ec == 0 ]]; then
    num_tests_passed=$((num_tests_passed + 1))
  else
    num_tests_failed=$((num_tests_failed + 1))
    failed_tests+=("$tool_test")
  fi
done

echo -e "\nTest Summary"
echo "Tests Passed: $num_tests_passed"
echo "Tests Failed: $num_tests_failed"

if [ -n "$failed_tests" ]; then
  echo "The following test(s) Failed:"
  for tests in "${failed_tests[@]}"; do
    echo "$tests" 
  done
fi

exit $num_tests_failed
