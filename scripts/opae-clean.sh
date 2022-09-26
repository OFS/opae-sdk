#!/bin/bash

shopt -o -s nounset

declare -ra BINS=(\
PACSign \
afu_json_mgr \
afu_platform_config \
afu_platform_info \
afu_synth_setup \
bist \
bist_app \
bist_app.py \
bist_common.py \
bist_def.py \
bist_dma.py \
bist_nlb0.py \
bist_nlb3.py \
coreidle \
dummy_afu \
fecmode \
fpga_dma_N3000_test \
fpga_dma_test \
fpgabist \
fpgaconf \
fpgad \
fpgadiag \
fpgainfo \
fpgalpbk \
fpgamac \
fpgametrics \
fpgastats \
fvlbypass \
hello_cxxcore \
hello_events \
hello_fpga \
host_exerciser \
hps \
hssi \
hssi_config \
hssi_loopback \
hssiloopback \
hssimac \
hssistats \
mactest \
mmlink \
n5010-ctl \
n5010-test \
nlb0 \
nlb3 \
nlb7 \
object_api \
opae.io \
opaeuiotest \
opaevfiotest \
pac_hssi_config.py \
packager \
rtl_src_config \
userclk \
vabtool\
)

declare -a BIN_DIRS_TO_CLEAN=(\
'/usr/local/bin'\
)

declare -ra LIBS=(\
"libbitstream.so*" \
"libfpgad-api.so*" \
"libfpgaperf_counter.so*" \
"libhssi-io.so*" \
"libmml-srv.so*" \
"libmml-stream.so*" \
"libofs.so*" \
"libofs_cpeng.so*" \
"libopae-c++-nlb.so*" \
"libopae-c++-utils.so*" \
"libopae-c.so*" \
"libopae-cxx-core.so*" \
"libopaemem.so*" \
"libopaeuio.so*" \
"libopaevfio.so*" \
"opae/libboard_a10gx.so*" \
"opae/libboard_d5005.so*" \
"opae/libboard_n3000.so*" \
"opae/libboard_n5010.so*" \
"opae/libboard_n6010.so*" \
"opae/libboard_n6000.so*" \
"opae/libfpgad-vc.so*" \
"opae/libfpgad-xfpga.so*" \
"opae/libmodbmc.so*" \
"opae/libopae-v.so*" \
"opae/libxfpga.so*"\
)

declare -a LIB_DIRS_TO_CLEAN=(\
'/usr/local/lib' \
'/usr/local/lib64'\
)

declare -i DELETE=0
clean() {
  if [ ${DELETE} -eq 1 ]; then
    if [ -f "$1" ]; then
      printf "rm -f $1\n"
      rm -f $1
    elif [ -d "$1" ]; then
      printf "rm -rf $1\n"
      rm -rf $1
    elif [ -L "$1" ]; then
      printf "unlink $1\n"
      unlink $1
    fi
  else
    printf "$1\n"
  fi
}

clean_bins() {
  for d in ${BIN_DIRS_TO_CLEAN[@]} ; do
    for b in ${BINS[@]} ; do
      if [ -f "$d/$b" ]; then
        clean "$d/$b"
      fi
    done
  done
}

clean_libs() {
  local glob_pattern
  local glob_result
  local item

  for d in ${LIB_DIRS_TO_CLEAN[@]} ; do
    for l in ${LIBS[@]} ; do
      glob_pattern="$d/$l"
      glob_result=(${glob_pattern})

      if [ "${glob_result[0]}" != "${glob_pattern}" ]; then
        for item in ${glob_result[@]} ; do
          if [ -f "${item}" -o -L "${item}" ]; then
            clean "${item}"
          fi
        done
      fi

    done
  done
}

declare -a PYTHON_DIRS_TO_CLEAN=(\
'/usr/lib' \
'/usr/lib64' \
'/usr/local/lib' \
'/usr/local/lib64'\
)

python_files() {
  local files

  local -ra dirs=('ethernet' \
                  'opae' \
                  'pacsign')

  local -ra globs=('hssi_ethernet-*.egg-info' \
                   'libvfio*.so' \
                   'opae.admin-*.egg-info' \
                   'opae.admin-*.pth' \
                   'opae.diag-*.egg-info' \
                   'opae.diag-*.pth' \
                   'opae.io-*.egg-info' \
                   'opae.io-*.pth' \
                   'pacsign-*.egg-info' \
                   'pacsign-*.pth' \
                   'pyopaeuio-*.egg-info' \
                   'pyopaeuio-*.pth' \
                   'pyopaeuio*.so')
  local py_glob
  local -a py_paths
  local py
  local files

  for d in ${PYTHON_DIRS_TO_CLEAN[@]} ; do
    py_glob="$d/python*/site-packages"
    py_paths=(${py_glob})

    if [ "${py_paths[0]}" != "${py_glob}" ]; then
      for py in ${py_paths[@]} ; do

        for e in ${dirs[@]} ; do
          [ -d "$py/$e" ] && printf "%s\n" "$py/$e"
        done

        for g in ${globs[@]} ; do
          glob=$py/$g
          files=(${glob})
          if [ "${files[0]}" != "${glob}" ]; then
            for f in ${files[@]} ; do
              printf "%s\n" "$f"
            done
          fi
        done

      done
    fi
  done
}

clean_python() {
  for f in `python_files` ; do
    clean "$f"
  done
}

declare -a CONF_HOME_FILES_TO_CLEAN=(\
'/.local/opae.cfg' \
'/.local/opae/opae.cfg' \
'/.config/opae/opae.cfg'\
)

declare -a CONF_SYS_FILES_TO_CLEAN=(\
'/usr/local/etc/opae/opae.cfg' \
'/etc/opae/opae.cfg'\
)

clean_configurations() {
  shopt -o -u nounset

  if ! [ -z ${LIBOPAE_CFGFILE} ]; then
    if [ -f "${LIBOPAE_CFGFILE}" ]; then
      clean "${LIBOPAE_CFGFILE}"
    fi
  fi

  shopt -o -s nounset

  for f in "${CONF_HOME_FILES_TO_CLEAN[@]}"; do
    if [ -f "${HOME}$f" ]; then
      clean "${HOME}$f"
    fi
  done

  for f in "${CONF_SYS_FILES_TO_CLEAN[@]}"; do
    if [ -f "$f" ]; then
      clean "$f"
    fi
  done
}

show_help() {
  printf "Usage: opae-clean [-b] [-l] [-p] [-h] [-c] [-d] [-B dir] [-L dir] [-P dir] [-X prefix]\n"
  printf "\n"
  printf "  -b          : clean OPAE binaries\n"
  printf "  -B <dir>    : specify additional bin directory\n"
  printf "  -l          : clean OPAE libraries\n"
  printf "  -L <dir>    : specify additional lib directory\n"
  printf "  -p          : clean OPAE Python artifacts\n"
  printf "  -P <dir>    : specify additional python directory\n"
  printf "  -c          : clean OPAE configuration files\n"
  printf "  -X <prefix> : specify a prefix to which /bin, /lib, and /lib64 will be added\n"
  printf "  -h          : display this help\n"
  printf "  -d          : yes, really delete the files\n"
}

declare -a CMDS=(x)

while getopts ":bcdlphB:L:P:X:" o; do
  case "$o" in
    b)
      CMDS=(${CMDS[@]} b)
    ;;

    c)
      CMDS=(${CMDS[@]} c)
    ;;

    d)
      DELETE=1
    ;;

    l)
      CMDS=(${CMDS[@]} l)
    ;;

    p)
      CMDS=(${CMDS[@]} p)
    ;;

    B)
      BIN_DIRS_TO_CLEAN=(${BIN_DIRS_TO_CLEAN[@]} ${OPTARG})
    ;;

    L)
      LIB_DIRS_TO_CLEAN=(${LIB_DIRS_TO_CLEAN[@]} ${OPTARG})
    ;;

    P)
      PYTHON_DIRS_TO_CLEAN=(${PYTHON_DIRS_TO_CLEAN[@]} ${OPTARG})
    ;;

    X)
      BIN_DIRS_TO_CLEAN=(${BIN_DIRS_TO_CLEAN[@]} ${OPTARG}/bin)
      LIB_DIRS_TO_CLEAN=(${LIB_DIRS_TO_CLEAN[@]} ${OPTARG}/lib)
      LIB_DIRS_TO_CLEAN=(${LIB_DIRS_TO_CLEAN[@]} ${OPTARG}/lib64)
    ;;

    h)
      show_help
      exit 1
    ;;
  esac
done

declare -i DID_SOMETHING=0

for c in ${CMDS[@]} ; do
  case "$c" in
    b)
      clean_bins
      DID_SOMETHING=1
    ;;

    c)
      clean_configurations
      DID_SOMETHING=1
    ;;

    l)
      clean_libs
      DID_SOMETHING=1
    ;;

    p)
      clean_python
      DID_SOMETHING=1
    ;;
  esac
done

if [ ${DID_SOMETHING} -eq 0 ]; then
  show_help
  exit 1
fi

exit 0
