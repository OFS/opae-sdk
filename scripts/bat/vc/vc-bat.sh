#!/bin/bash
#
# Perform quick BAT test on VC tools

readonly OUTPUT_MACRO=(
"Board Power" "12V Backplane Current" "12V Backplane Voltage"
"1.2V Voltage" "1.8V Voltage" "3.3V Voltage" "FPGA Core Voltage"
"FPGA Core Current" "FPGA Die Temperature" "Board Temperature"
"QSFP0 Supply Voltage" "QSFP0 Temperature" "12V AUX Current"
"12V AUX Voltage" "QSFP1 Supply Voltage" "QSFP1 Temperature"
"PKVL0 Core Temperature" "PKVL0 SerDes Temperature"
"PKVL1 Core Temperature" "PKVL1 SerDes Temperature"
)

################################################
# Check fpgainfo argment to parse/compare output
# Globals:
#   parse_ec
# Arguments:
#   fpgainfo cmd
#   fpgainfo cmd output
# Returns:
#   None
################################################

# TODO(jelonanx): Refactor output parsing
parse_fpgainfo_output() {
  case $1 in 
    *"bmc"*)
         if [[ $2 =~ ${OUTPUT_MACRO[0]} && $2 =~ ${OUTPUT_MACRO[1]} && 
               $2 =~ ${OUTPUT_MACRO[2]} && $2 =~ ${OUTPUT_MACRO[3]} && 
               $2 =~ ${OUTPUT_MACRO[4]} && $2 =~ ${OUTPUT_MACRO[5]} && 
               $2 =~ ${OUTPUT_MACRO[6]} && $2 =~ ${OUTPUT_MACRO[7]} && 
               $2 =~ ${OUTPUT_MACRO[8]} && $2 =~ ${OUTPUT_MACRO[9]} && 
               $2 =~ ${OUTPUT_MACRO[10]} && $2 =~ ${OUTPUT_MACRO[11]} && 
               $2 =~ ${OUTPUT_MACRO[12]} && $2 =~ ${OUTPUT_MACRO[13]} && 
               $2 =~ ${OUTPUT_MACRO[14]} && $2 =~ ${OUTPUT_MACRO[15]} && 
               $2 =~ ${OUTPUT_MACRO[16]} && $2 =~ ${OUTPUT_MACRO[17]} && 
               $2 =~ ${OUTPUT_MACRO[18]} && $2 =~ ${OUTPUT_MACRO[19]} ]]; then
             parse_ec=0
         else
             parse_ec=1 
         fi 
         ;;
    *"temp"*)
         if [[ $2 =~ ${OUTPUT_MACRO[8]} && $2 =~ ${OUTPUT_MACRO[9]} && 
               $2 =~ ${OUTPUT_MACRO[11]} && $2 =~ ${OUTPUT_MACRO[15]} && 
               $2 =~ ${OUTPUT_MACRO[16]} && $2 =~ ${OUTPUT_MACRO[17]} && 
               $2 =~ ${OUTPUT_MACRO[18]} && $2 =~ ${OUTPUT_MACRO[19]} ]]; then
             parse_ec=0
         else
             parse_ec=1 
         fi 
         ;;
    *"power"*)
         if [[ $2 =~ ${OUTPUT_MACRO[0]} && $2 =~ ${OUTPUT_MACRO[1]} && 
               $2 =~ ${OUTPUT_MACRO[2]} && $2 =~ ${OUTPUT_MACRO[3]} && 
               $2 =~ ${OUTPUT_MACRO[4]} && $2 =~ ${OUTPUT_MACRO[5]} && 
               $2 =~ ${OUTPUT_MACRO[6]} && $2 =~ ${OUTPUT_MACRO[7]} && 
               $2 =~ ${OUTPUT_MACRO[10]} && $2 =~ ${OUTPUT_MACRO[12]} && 
               $2 =~ ${OUTPUT_MACRO[13]} && $2 =~ ${OUTPUT_MACRO[14]} ]]; then
             parse_ec=0
         else
             parse_ec=1
         fi 
         ;;
    *"phy"*)
         if [[ $2 =~ "PHY GROUP 0" && $2 =~ "PHY GROUP 1" && 
               $2 =~ "Intel C827 Retimer" && 
               $2 =~ "Port0" && $2 =~ "Port1" ]]; then
             parse_ec=0
         else
             parse_ec=1
         fi 
         ;;
    *"errors"*)
         if [[ $2 =~ "Errors" && $2 =~ "Next Error" && $2 =~ "First Error" && 
               $2 =~ "PCIe0 Errors" && $2 =~ "Inject Error" && 
               $2 =~ "Catfatal Errors" && $2 =~ "Nonfatal Errors" && 
               $2 =~ "PCIe1 Errors" ]]; then
             parse_ec=0
         else
             parse_ec=1
         fi 
         ;;
    *"mac"*)
         if [[ $2 =~ "Number of MACs" && $2 =~ "MAC address 0" && 
               $2 =~ "MAC address 1" && $2 =~ "MAC address 2" &&
               $2 =~ "MAC address 3" && $2 =~ "MAC address 4" &&
               $2 =~ "MAC address 5" && $2 =~ "MAC address 6" &&
               $2 =~ "MAC address 7" ]]; then
             parse_ec=0
         else
             parse_ec=1
         fi 
         ;;
    *"fme"*|*"port"*)
         parse_ec=0 
         ;;
    *)
         echo "Invalid fpgainfo argument."
         parse_ec=1 
         ;;
  esac
}

found=$(ls /dev/intel-* | wc -l)
if [[ $found -lt 1 ]]; then
    echo "Failed to locate drivers."
    exit 1
fi

found=$(lspci | grep 0b30)
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

tool_tests=(
"fpgainfo errors all -B 0x$bus_num"
"fpgainfo fme -B 0x$bus_num"
"fpgainfo port -B 0x$bus_num"
"fpgainfo mac -B 0x$bus_num"
"fpgainfo phy -B 0x$bus_num"
"fpgainfo temp -B 0x$bus_num"
"fpgainfo bmc -B 0x$bus_num"
"fpgainfo power -B 0x$bus_num"
"fpgalpbk -G 9AEFFE5F-8457-0612-C000-C9660D824272 -B 0x$bus_num --side=line --direction=local --enable"
"fpgalpbk -G 9AEFFE5F-8457-0612-C000-C9660D824272 -B 0x$bus_num --side=line --direction=local --disable"
"fpgalpbk -G 9AEFFE5F-8457-0612-C000-C9660D824272 -I 9AEFFE5F-8457-0612-C000-C9660D824272 -B 0x$bus_num --side=line --direction=local --enable"
"fpgalpbk -G 9AEFFE5F-8457-0612-C000-C9660D824272 -I 9AEFFE5F-8457-0612-C000-C9660D824272 -B 0x$bus_num --side=line --direction=local --disable"
"fpgalpbk -G 9AEFFE5F-8457-0612-C000-C9660D824272 -B 0x$bus_num --side=host --direction=remote --enable"
"fpgalpbk -G 9AEFFE5F-8457-0612-C000-C9660D824272 -B 0x$bus_num --side=host --direction=remote --disable"
"fpga_dma_test 0 -B 0x$bus_num -G 9AEFFE5F-8457-0612-C000-C9660D824272 -D 0 -S 0x100000000"
"fpga_dma_test 0 -B 0x$bus_num -G 9AEFFE5F-8457-0612-C000-C9660D824272 -D 1 -S 0x100000000"
"fpga_dma_test 0 -B 0x$bus_num -G 9AEFFE5F-8457-0612-C000-C9660D824272 -D 2 -S 0x40000000"
"fpga_dma_test 0 -B 0x$bus_num -G 9AEFFE5F-8457-0612-C000-C9660D824272 -D 3 -S 0x1000000"
"nlb0 --guid 9AEFFE5F-8457-0612-C000-C9660D824272 -B 0x$bus_num --multi-cl=4 --begin=1024 --end=1024 --timeout-sec=1 --cont" 
"nlb0 --guid 9AEFFE5F-8457-0612-C000-C9660D824272 -B 0x$bus_num"
"nlb0 --guid 9AEFFE5F-8457-0612-C000-C9660D824272 -B 0x$bus_num --cont"
"nlb0 --guid 9AEFFE5F-8457-0612-C000-C9660D824272 -B 0x$bus_num --begin 10 --end 1000"
"nlb0 --guid 9AEFFE5F-8457-0612-C000-C9660D824272 -B 0x$bus_num --begin 10 --end 100 --suppress-hdr -V"
"nlb0 --guid 9AEFFE5F-8457-0612-C000-C9660D824272 -B 0x$bus_num --begin 10 --end 100 --suppress-stats -V"
"nlb0 --guid 9AEFFE5F-8457-0612-C000-C9660D824272 -B 0x$bus_num --multi-cl 2 --begin 10 --end 1000"
"nlb0 --guid 9AEFFE5F-8457-0612-C000-C9660D824272 -B 0x$bus_num --begin 2050 --cont --timeout-sec 5"
"nlb0 -V --guid 9AEFFE5F-8457-0612-C000-C9660D824272 -B 0x$bus_num --begin=57535 --end=65535 --cache-hint=rdline-I --cache-policy=wrpush-I --write-vc=vl0 --read-vc=vh1 --wrfence-vc=random"
"nlb0 -V --guid 9AEFFE5F-8457-0612-C000-C9660D824272 -B 0x$bus_num --begin=57532 --end=65532 --multi-cl 4 --cache-hint=rdline-I --cache-policy=wrline-I --write-vc=vh0 --read-vc=vl0 --wrfence-vc=auto"
"fpgastats --guid 9AEFFE5F-8457-0612-C000-C9660D824272 -B 0x$bus_num"
"mactest -B 0x$bus_num"
"mactest -B 0x$bus_num --offset=0x100"
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
