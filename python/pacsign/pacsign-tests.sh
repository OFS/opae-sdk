#!/bin/bash
# Copyright(c) 2021, Intel Corporation
#
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of  source code  must retain the  above copyright notice,
#   this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name  of Intel Corporation  nor the names of its contributors
#   may be used to  endorse or promote  products derived  from this  software
#   without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
# IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
# LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
# CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
# SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
# INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
# CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

shopt -o -s nounset
set -e

die() {
  printf "%s" "$*"
  exit 1
}

declare -r PACSIGN=`which PACSign 2>/dev/null`
[ "x${PACSIGN}" = "x" ] && die 'PACSign not found'

declare -r OPENSSL=`which openssl 2>/dev/null`
[ "x${OPENSSL}" = "x" ] && die 'openssl not found'

declare -r INFILE='file'
declare -r KEYSTORE='keys'
declare -r OUTPUT='out'

create_keys_() {
  local -r img="$1"
  local -r curve="$2"
  local width='256'

  [ "${curve}" = "secp384r1" ] && width='384'

  [ -d "${KEYSTORE}/${img}" ] || mkdir -p "${KEYSTORE}/${img}"

  if ! [ -f "${KEYSTORE}/${img}/test_key_${img}_root_private_${width}.pem" ]; then
    ${OPENSSL} ecparam -name "${curve}" -genkey -noout -out "${KEYSTORE}/${img}/test_key_${img}_root_private_${width}.pem"
  fi
  if ! [ -f "${KEYSTORE}/${img}/test_key_${img}_root_public_${width}.pem" ]; then
    ${OPENSSL} ec -in "${KEYSTORE}/${img}/test_key_${img}_root_private_${width}.pem" \
               -pubout -out "${KEYSTORE}/${img}/test_key_${img}_root_public_${width}.pem"
  fi

  if ! [ -f "${KEYSTORE}/${img}/test_key_${img}_csk0_private_${width}.pem" ]; then
    ${OPENSSL} ecparam -name "${curve}" -genkey -noout -out "${KEYSTORE}/${img}/test_key_${img}_csk0_private_${width}.pem"
  fi
  if ! [ -f "${KEYSTORE}/${img}/test_key_${img}_csk0_public_${width}.pem" ]; then
    ${OPENSSL} ec -in "${KEYSTORE}/${img}/test_key_${img}_csk0_private_${width}.pem" \
               -pubout -out "${KEYSTORE}/${img}/test_key_${img}_csk0_public_${width}.pem"
  fi
}

create_keys() {
  create_keys_ fim         prime256v1
  create_keys_ fim2        prime256v1
  create_keys_ sr_test     prime256v1
  create_keys_ sr_cert     prime256v1
  create_keys_ bmc         prime256v1
  create_keys_ bmc_factory prime256v1
  create_keys_ pr          prime256v1
  create_keys_ pr_test     prime256v1
  create_keys_ pxe         prime256v1
  create_keys_ therm_sr    prime256v1
  create_keys_ therm_pr    prime256v1
  create_keys_ sdm         prime256v1

  create_keys_ fim         secp384r1 
  create_keys_ fim2        secp384r1 
  create_keys_ sr_test     secp384r1 
  create_keys_ sr_cert     secp384r1 
  create_keys_ bmc         secp384r1 
  create_keys_ bmc_factory secp384r1 
  create_keys_ pr          secp384r1 
  create_keys_ pr_test     secp384r1 
  create_keys_ pxe         secp384r1 
  create_keys_ therm_sr    secp384r1 
  create_keys_ therm_pr    secp384r1 
  create_keys_ sdm         secp384r1 
}

################################################################################

image_type_to_PACSign() {
  local -r from="$1"
  case "${from}" in
    fim)
      printf "FIM"
    ;;

    fim2)
      printf "FACTORY"
    ;;

    bmc)
      printf "BMC"
    ;;

    bmc_factory)
      printf "BMC_FACTORY"
    ;;

    pr)
      printf "PR"
    ;;

    pr_test)
      printf "PR_TEST"
    ;;

    sr_test)
      printf "SR_TEST"
    ;;

    sr_cert)
      printf "SR_CERT"
    ;;

    pxe)
      printf "PXE"
    ;;

    therm_sr)
      printf "THERM_SR"
    ;;

    therm_pr)
      printf "THERM_PR"
    ;;

    sdm)
      printf "SDM"
    ;;
  
   esac
}

# Create Root Key Hash
test_rkh() {
  local -r mgr="$1"
  local -r img="$2"
  local -ri width=$3
  local extra=''
  local img_type=$(image_type_to_PACSign "${img}")

  [ -d "${OUTPUT}/${img}" ] || mkdir -p "${OUTPUT}/${img}" 
  [ ${width} -eq 384 ] && extra='S'

  ${PACSIGN} "${img_type}" -yv"${extra}" -b "RKH_${img}_${width}" -t "RK_${width}" \
             -H "${mgr}" \
             -r "${KEYSTORE}/${img}/test_key_${img}_root_public_${width}.pem" \
             -o "${OUTPUT}/${img}/test_rkh_${img}_${width}.bin"
}

# Create 'unsigned' bitstream
test_unsigned() {
  local -r mgr="$1"
  local -r img="$2"
  local -ri width=$3
  local -ri slot=$4
  local extra=''
  local img_type=$(image_type_to_PACSign "${img}")

  [ -d "${OUTPUT}/${img}" ] || mkdir -p "${OUTPUT}/${img}" 
  [ ${width} -eq 384 ] && extra='S'

  ${PACSIGN} "${img_type}" -yv"${extra}" -b "unsigned_${img}_${width}" -s ${slot} -t UPDATE \
             -H "${mgr}" \
             -i "${INFILE}" \
             -o "${OUTPUT}/${img}/${INFILE}.${img}.slot${slot}.${width}.unsigned.bin"
}

# Create signed bitstream
test_signed() {
  local -r mgr="$1"
  local -r img="$2"
  local -ri width=$3
  local -ri slot=$4
  local extra=''
  local img_type=$(image_type_to_PACSign "${img}")

  [ -d "${OUTPUT}/${img}" ] || mkdir -p "${OUTPUT}/${img}" 
  [ ${width} -eq 384 ] && extra='S'

  ${PACSIGN} "${img_type}" -yv"${extra}" -b "signed_${img}_${width}" -s ${slot} -t UPDATE \
    -H "${mgr}" \
    -r "${KEYSTORE}/${img}/test_key_${img}_root_public_${width}.pem" \
    -k "${KEYSTORE}/${img}/test_key_${img}_csk0_public_${width}.pem" \
    -R "${OUTPUT}/${img}/test_rkh_${img}_${width}.bin" \
    -i "${INFILE}" \
    -o "${OUTPUT}/${img}/${INFILE}.${img}.slot${slot}.${width}.signed.bin"
}

# Create Code Signing Key cancellation
test_cancel() {
  local -r mgr="$1"
  local -r img="$2"
  local -ri width=$3
  local -ri csk=$4
  local extra=''
  local img_type=$(image_type_to_PACSign "${img}")

  [ -d "${OUTPUT}/${img}" ] || mkdir -p "${OUTPUT}/${img}" 
  [ ${width} -eq 384 ] && extra='S'

  ${PACSIGN} "${img_type}" -yv"${extra}" -b "cancel${csk}_${img}_${width}" -t CANCEL \
    -H "${mgr}" \
    -r "${KEYSTORE}/${img}/test_key_${img}_root_public_${width}.pem" \
    -d ${csk} \
    -o "${OUTPUT}/${img}/${INFILE}.${img}.${width}.cancel_csk${csk}.bin"
}

################################################################################
# FIM

test_fim_rkh() {
  local -r mgr="$1"
  printf "test_fim_rkh\n"
  test_rkh "${mgr}" 'fim' 256
  test_rkh "${mgr}" 'fim' 384
}

test_fim_unsigned() {
  local -r mgr="$1"
  printf "test_fim_unsigned\n"
  test_unsigned "${mgr}" 'fim' 256 0
  test_unsigned "${mgr}" 'fim' 256 1
  test_unsigned "${mgr}" 'fim' 384 0
  test_unsigned "${mgr}" 'fim' 384 1
}

test_fim_signed() {
  local -r mgr="$1"
  printf "test_fim_signed\n"
  test_signed "${mgr}" 'fim' 256 0
  test_signed "${mgr}" 'fim' 256 1
  test_signed "${mgr}" 'fim' 384 0
  test_signed "${mgr}" 'fim' 384 1
}

test_fim_cancel() {
  local -r manager="$1"
  printf "test_fim_cancel\n"
  test_cancel "${mgr}" 'fim' 256 0
  test_cancel "${mgr}" 'fim' 256 0
  test_cancel "${mgr}" 'fim' 384 0
  test_cancel "${mgr}" 'fim' 384 0
}

################################################################################
# FACTORY

test_factory_rkh() {
  local -r mgr="$1"
  printf "test_factory_rkh\n"
  test_rkh "${mgr}" 'fim2' 256
  test_rkh "${mgr}" 'fim2' 384
}

test_factory_unsigned() {
  local -r mgr="$1"
  printf "test_factory_unsigned\n"
  test_unsigned "${mgr}" 'fim2' 256 0
  test_unsigned "${mgr}" 'fim2' 256 1
  test_unsigned "${mgr}" 'fim2' 384 0
  test_unsigned "${mgr}" 'fim2' 384 1
}

test_factory_signed() {
  local -r mgr="$1"
  printf "test_factory_signed\n"
  test_signed "${mgr}" 'fim2' 256 0
  test_signed "${mgr}" 'fim2' 256 1
  test_signed "${mgr}" 'fim2' 384 0
  test_signed "${mgr}" 'fim2' 384 1
}

test_factory_cancel() {
  local -r manager="$1"
  printf "test_factory_cancel\n"
  test_cancel "${mgr}" 'fim2' 256 0
  test_cancel "${mgr}" 'fim2' 256 0
  test_cancel "${mgr}" 'fim2' 384 0
  test_cancel "${mgr}" 'fim2' 384 0
}

################################################################################
# SR_TEST

test_sr_test_rkh() {
  local -r mgr="$1"
  printf "test_sr_test_rkh\n"
  test_rkh "${mgr}" 'sr_test' 256
  test_rkh "${mgr}" 'sr_test' 384
}

test_sr_test_unsigned() {
  local -r mgr="$1"
  printf "test_sr_test_unsigned\n"
  test_unsigned "${mgr}" 'sr_test' 256 0
  test_unsigned "${mgr}" 'sr_test' 256 1
  test_unsigned "${mgr}" 'sr_test' 384 0
  test_unsigned "${mgr}" 'sr_test' 384 1
}

test_sr_test_signed() {
  local -r mgr="$1"
  printf "test_sr_test_signed\n"
  test_signed "${mgr}" 'sr_test' 256 0
  test_signed "${mgr}" 'sr_test' 256 1
  test_signed "${mgr}" 'sr_test' 384 0
  test_signed "${mgr}" 'sr_test' 384 1
}

test_sr_test_cancel() {
  local -r manager="$1"
  printf "test_sr_test_cancel\n"
  test_cancel "${mgr}" 'sr_test' 256 0
  test_cancel "${mgr}" 'sr_test' 256 0
  test_cancel "${mgr}" 'sr_test' 384 0
  test_cancel "${mgr}" 'sr_test' 384 0
}

################################################################################
# SR_CERT

test_sr_cert_rkh() {
  local -r mgr="$1"
  printf "test_sr_cert_rkh\n"
  test_rkh "${mgr}" 'sr_cert' 256
  test_rkh "${mgr}" 'sr_cert' 384
}

test_sr_cert_unsigned() {
  local -r mgr="$1"
  printf "test_sr_cert_unsigned\n"
  test_unsigned "${mgr}" 'sr_cert' 256 0
  test_unsigned "${mgr}" 'sr_cert' 256 1
  test_unsigned "${mgr}" 'sr_cert' 384 0
  test_unsigned "${mgr}" 'sr_cert' 384 1
}

test_sr_cert_signed() {
  local -r mgr="$1"
  printf "test_sr_cert_signed\n"
  test_signed "${mgr}" 'sr_cert' 256 0
  test_signed "${mgr}" 'sr_cert' 256 1
  test_signed "${mgr}" 'sr_cert' 384 0
  test_signed "${mgr}" 'sr_cert' 384 1
}

test_sr_cert_cancel() {
  local -r manager="$1"
  printf "test_sr_cert_cancel\n"
  test_cancel "${mgr}" 'sr_cert' 256 0
  test_cancel "${mgr}" 'sr_cert' 256 0
  test_cancel "${mgr}" 'sr_cert' 384 0
  test_cancel "${mgr}" 'sr_cert' 384 0
}

################################################################################
# BMC

test_bmc_rkh() {
  local -r mgr="$1"
  printf "test_bmc_rkh\n"
  test_rkh "${mgr}" 'bmc' 256
  test_rkh "${mgr}" 'bmc' 384
}

test_bmc_unsigned() {
  local -r mgr="$1"
  printf "test_bmc_unsigned\n"
  test_unsigned "${mgr}" 'bmc' 256 0
  test_unsigned "${mgr}" 'bmc' 256 1
  test_unsigned "${mgr}" 'bmc' 384 0
  test_unsigned "${mgr}" 'bmc' 384 1
}

test_bmc_signed() {
  local -r mgr="$1"
  printf "test_bmc_signed\n"
  test_signed "${mgr}" 'bmc' 256 0
  test_signed "${mgr}" 'bmc' 256 1
  test_signed "${mgr}" 'bmc' 384 0
  test_signed "${mgr}" 'bmc' 384 1
}

test_bmc_cancel() {
  local -r manager="$1"
  printf "test_bmc_cancel\n"
  test_cancel "${mgr}" 'bmc' 256 0
  test_cancel "${mgr}" 'bmc' 256 0
  test_cancel "${mgr}" 'bmc' 384 0
  test_cancel "${mgr}" 'bmc' 384 0
}

################################################################################
# BMC_FACTORY

test_bmc_factory_rkh() {
  local -r mgr="$1"
  printf "test_bmc_factory_rkh\n"
  test_rkh "${mgr}" 'bmc_factory' 256
  test_rkh "${mgr}" 'bmc_factory' 384
}

test_bmc_factory_unsigned() {
  local -r mgr="$1"
  printf "test_bmc_factory_unsigned\n"
  test_unsigned "${mgr}" 'bmc_factory' 256 0
  test_unsigned "${mgr}" 'bmc_factory' 256 1
  test_unsigned "${mgr}" 'bmc_factory' 384 0
  test_unsigned "${mgr}" 'bmc_factory' 384 1
}

test_bmc_factory_signed() {
  local -r mgr="$1"
  printf "test_bmc_factory_signed\n"
  test_signed "${mgr}" 'bmc_factory' 256 0
  test_signed "${mgr}" 'bmc_factory' 256 1
  test_signed "${mgr}" 'bmc_factory' 384 0
  test_signed "${mgr}" 'bmc_factory' 384 1
}

test_bmc_factory_cancel() {
  local -r manager="$1"
  printf "test_bmc_factory_cancel\n"
  test_cancel "${mgr}" 'bmc_factory' 256 0
  test_cancel "${mgr}" 'bmc_factory' 256 0
  test_cancel "${mgr}" 'bmc_factory' 384 0
  test_cancel "${mgr}" 'bmc_factory' 384 0
}

################################################################################
# PR

test_pr_rkh() {
  local -r mgr="$1"
  printf "test_pr_rkh\n"
  test_rkh "${mgr}" 'pr' 256
  test_rkh "${mgr}" 'pr' 384
}

test_pr_unsigned() {
  local -r mgr="$1"
  printf "test_pr_unsigned\n"
  test_unsigned "${mgr}" 'pr' 256 0
  test_unsigned "${mgr}" 'pr' 256 1
  test_unsigned "${mgr}" 'pr' 384 0
  test_unsigned "${mgr}" 'pr' 384 1
}

test_pr_signed() {
  local -r mgr="$1"
  printf "test_pr_signed\n"
  test_signed "${mgr}" 'pr' 256 0
  test_signed "${mgr}" 'pr' 256 1
  test_signed "${mgr}" 'pr' 384 0
  test_signed "${mgr}" 'pr' 384 1
}

test_pr_cancel() {
  local -r manager="$1"
  printf "test_pr_cancel\n"
  test_cancel "${mgr}" 'pr' 256 0
  test_cancel "${mgr}" 'pr' 256 0
  test_cancel "${mgr}" 'pr' 384 0
  test_cancel "${mgr}" 'pr' 384 0
}

################################################################################
# PR_TEST

test_pr_test_rkh() {
  local -r mgr="$1"
  printf "test_pr_test_rkh\n"
  test_rkh "${mgr}" 'pr_test' 256
  test_rkh "${mgr}" 'pr_test' 384
}

test_pr_test_unsigned() {
  local -r mgr="$1"
  printf "test_pr_test_unsigned\n"
  test_unsigned "${mgr}" 'pr_test' 256 0
  test_unsigned "${mgr}" 'pr_test' 256 1
  test_unsigned "${mgr}" 'pr_test' 384 0
  test_unsigned "${mgr}" 'pr_test' 384 1
}

test_pr_test_signed() {
  local -r mgr="$1"
  printf "test_pr_test_signed\n"
  test_signed "${mgr}" 'pr_test' 256 0
  test_signed "${mgr}" 'pr_test' 256 1
  test_signed "${mgr}" 'pr_test' 384 0
  test_signed "${mgr}" 'pr_test' 384 1
}

test_pr_test_cancel() {
  local -r manager="$1"
  printf "test_pr_test_cancel\n"
  test_cancel "${mgr}" 'pr_test' 256 0
  test_cancel "${mgr}" 'pr_test' 256 0
  test_cancel "${mgr}" 'pr_test' 384 0
  test_cancel "${mgr}" 'pr_test' 384 0
}

################################################################################
# PXE

test_pxe_rkh() {
  local -r mgr="$1"
  printf "test_pxe_rkh\n"
  test_rkh "${mgr}" 'pxe' 256
  test_rkh "${mgr}" 'pxe' 384
}

test_pxe_unsigned() {
  local -r mgr="$1"
  printf "test_pxe_unsigned\n"
  test_unsigned "${mgr}" 'pxe' 256 0
  test_unsigned "${mgr}" 'pxe' 256 1
  test_unsigned "${mgr}" 'pxe' 384 0
  test_unsigned "${mgr}" 'pxe' 384 1
}

test_pxe_signed() {
  local -r mgr="$1"
  printf "test_pxe_signed\n"
  test_signed "${mgr}" 'pxe' 256 0
  test_signed "${mgr}" 'pxe' 256 1
  test_signed "${mgr}" 'pxe' 384 0
  test_signed "${mgr}" 'pxe' 384 1
}

test_pxe_cancel() {
  local -r manager="$1"
  printf "test_pxe_cancel\n"
  test_cancel "${mgr}" 'pxe' 256 0
  test_cancel "${mgr}" 'pxe' 256 0
  test_cancel "${mgr}" 'pxe' 384 0
  test_cancel "${mgr}" 'pxe' 384 0
}

################################################################################
# THERM_SR

test_therm_sr_rkh() {
  local -r mgr="$1"
  printf "test_therm_sr_rkh\n"
  test_rkh "${mgr}" 'therm_sr' 256
  test_rkh "${mgr}" 'therm_sr' 384
}

test_therm_sr_unsigned() {
  local -r mgr="$1"
  printf "test_therm_sr_unsigned\n"
  test_unsigned "${mgr}" 'therm_sr' 256 0
  test_unsigned "${mgr}" 'therm_sr' 256 1
  test_unsigned "${mgr}" 'therm_sr' 384 0
  test_unsigned "${mgr}" 'therm_sr' 384 1
}

test_therm_sr_signed() {
  local -r mgr="$1"
  printf "test_therm_sr_signed\n"
  test_signed "${mgr}" 'therm_sr' 256 0
  test_signed "${mgr}" 'therm_sr' 256 1
  test_signed "${mgr}" 'therm_sr' 384 0
  test_signed "${mgr}" 'therm_sr' 384 1
}

test_therm_sr_cancel() {
  local -r manager="$1"
  printf "test_therm_sr_cancel\n"
  test_cancel "${mgr}" 'therm_sr' 256 0
  test_cancel "${mgr}" 'therm_sr' 256 0
  test_cancel "${mgr}" 'therm_sr' 384 0
  test_cancel "${mgr}" 'therm_sr' 384 0
}

################################################################################
# THERM_PR

test_therm_pr_rkh() {
  local -r mgr="$1"
  printf "test_therm_pr_rkh\n"
  test_rkh "${mgr}" 'therm_pr' 256
  test_rkh "${mgr}" 'therm_pr' 384
}

test_therm_pr_unsigned() {
  local -r mgr="$1"
  printf "test_therm_pr_unsigned\n"
  test_unsigned "${mgr}" 'therm_pr' 256 0
  test_unsigned "${mgr}" 'therm_pr' 256 1
  test_unsigned "${mgr}" 'therm_pr' 384 0
  test_unsigned "${mgr}" 'therm_pr' 384 1
}

test_therm_pr_signed() {
  local -r mgr="$1"
  printf "test_therm_pr_signed\n"
  test_signed "${mgr}" 'therm_pr' 256 0
  test_signed "${mgr}" 'therm_pr' 256 1
  test_signed "${mgr}" 'therm_pr' 384 0
  test_signed "${mgr}" 'therm_pr' 384 1
}

test_therm_pr_cancel() {
  local -r manager="$1"
  printf "test_therm_pr_cancel\n"
  test_cancel "${mgr}" 'therm_pr' 256 0
  test_cancel "${mgr}" 'therm_pr' 256 0
  test_cancel "${mgr}" 'therm_pr' 384 0
  test_cancel "${mgr}" 'therm_pr' 384 0
}

################################################################################
# SDM

test_sdm_rkh() {
  local -r mgr="$1"
  printf "test_sdm_rkh\n"
  test_rkh "${mgr}" 'sdm' 256
  test_rkh "${mgr}" 'sdm' 384
}

test_sdm_unsigned() {
  local -r mgr="$1"
  printf "test_sdm_unsigned\n"
  test_unsigned "${mgr}" 'sdm' 256 0
  test_unsigned "${mgr}" 'sdm' 256 1
  test_unsigned "${mgr}" 'sdm' 384 0
  test_unsigned "${mgr}" 'sdm' 384 1
}

test_sdm_signed() {
  local -r mgr="$1"
  printf "test_sdm_signed\n"
  test_signed "${mgr}" 'sdm' 256 0
  test_signed "${mgr}" 'sdm' 256 1
  test_signed "${mgr}" 'sdm' 384 0
  test_signed "${mgr}" 'sdm' 384 1
}

test_sdm_cancel() {
  local -r manager="$1"
  printf "test_sdm_cancel\n"
  test_cancel "${mgr}" 'sdm' 256 0
  test_cancel "${mgr}" 'sdm' 256 0
  test_cancel "${mgr}" 'sdm' 384 0
  test_cancel "${mgr}" 'sdm' 384 0
}

################################################################################

run_tests() {
  local mgr
  [ -f "${INFILE}" ] || dd if=/dev/urandom of="${INFILE}" bs=512 count=1
  [ -d "${KEYSTORE}" ] || mkdir -p "${KEYSTORE}"
  [ -d "${OUTPUT}" ] || mkdir -p "${OUTPUT}"
  create_keys

  for mgr in 'openssl_manager' ; do
    test_fim_rkh "${mgr}"
    test_fim_unsigned "${mgr}"
    test_fim_signed "${mgr}"
    test_fim_cancel "${mgr}"

    test_factory_rkh "${mgr}"
    test_factory_unsigned "${mgr}"
    test_factory_signed "${mgr}"
    test_factory_cancel "${mgr}"

    test_sr_test_rkh "${mgr}"
    test_sr_test_unsigned "${mgr}"
    test_sr_test_signed "${mgr}"
    test_sr_test_cancel "${mgr}"

    test_sr_cert_rkh "${mgr}"
    test_sr_cert_unsigned "${mgr}"
    test_sr_cert_signed "${mgr}"
    test_sr_cert_cancel "${mgr}"

    test_bmc_rkh "${mgr}"
    test_bmc_unsigned "${mgr}"
    test_bmc_signed "${mgr}"
    test_bmc_cancel "${mgr}"

    test_bmc_factory_rkh "${mgr}"
    test_bmc_factory_unsigned "${mgr}"
    test_bmc_factory_signed "${mgr}"
    test_bmc_factory_cancel "${mgr}"

    test_pr_rkh "${mgr}"
    test_pr_unsigned "${mgr}"
    test_pr_signed "${mgr}"
    test_pr_cancel "${mgr}"

    test_pr_test_rkh "${mgr}"
    test_pr_test_unsigned "${mgr}"
    test_pr_test_signed "${mgr}"
    test_pr_test_cancel "${mgr}"

    test_pxe_rkh "${mgr}"
    test_pxe_unsigned "${mgr}"
    test_pxe_signed "${mgr}"
    test_pxe_cancel "${mgr}"

    test_therm_sr_rkh "${mgr}"
    test_therm_sr_unsigned "${mgr}"
    test_therm_sr_signed "${mgr}"
    test_therm_sr_cancel "${mgr}"

    test_therm_pr_rkh "${mgr}"
    test_therm_pr_unsigned "${mgr}"
    test_therm_pr_signed "${mgr}"
    test_therm_pr_cancel "${mgr}"

    test_sdm_rkh "${mgr}"
    test_sdm_unsigned "${mgr}"
    test_sdm_signed "${mgr}"
    test_sdm_cancel "${mgr}"
  done
}

clean_tests() {
  rm -rf "${KEYSTORE}" "${OUTPUT}"
  rm -f "${INFILE}"
  printf "clean\n"
}

declare -a CMDS=(x)

while getopts ":ch" o; do
  case "$o" in
    c)
      CMDS=(${CMDS[@]} c)
    ;;
    h)
      CMDS=(${CMDS[@]} h)
    ;;
  esac
done

declare -i DID_SOMETHING=0

for c in ${CMDS[@]} ; do
  case "$c" in
    c)
      clean_tests
      DID_SOMETHING=1
    ;;
    h)
      printf "usage: pacsign-tests.sh [-c] [-h]\n"
      printf "\n"
      printf "  When run without parameters, the tests are executed.\n"
      printf "  [-c] : clean all test output\n"
      printf "  [-h] : display this message and exit\n"
      exit 1
    ;;
  esac
done

if [ ${DID_SOMETHING} -eq 0 ]; then
  run_tests
fi

exit 0
