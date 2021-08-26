#!/bin/bash

shopt -o -s nounset

declare -i VER_MAJ=2
declare -i VER_MIN=0
declare -i VER_REL=7

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
n5010-ddr-test \
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
userclk\
)

declare -ra BIN_DIRS_TO_CLEAN=(\
'/usr/local/bin'\
)

declare -ra LIBS=(\
"libbitstream.so" \
"libbitstream.so.${VER_MAJ}" \
"libbitstream.so.${VER_MAJ}.${VER_MIN}.${VER_REL}" \
"libfpgad-api.so" \
"libfpgad-api.so.${VER_MAJ}" \
"libfpgad-api.so.${VER_MAJ}.${VER_MIN}.${VER_REL}" \
"libfpgaperf_counter.so" \
"libhssi-io.so" \
"libhssi-io.so.${VER_MAJ}" \
"libhssi-io.so.${VER_MAJ}.${VER_MIN}.${VER_REL}" \
"libmml-srv.so" \
"libmml-srv.so.${VER_MAJ}" \
"libmml-srv.so.${VER_MAJ}.${VER_MIN}.${VER_REL}" \
"libmml-stream.so" \
"libmml-stream.so.${VER_MAJ}" \
"libmml-stream.so.${VER_MAJ}.${VER_MIN}.${VER_REL}" \
"libofs.so" \
"libofs.so.${VER_MAJ}" \
"libofs.so.${VER_MAJ}.${VER_MIN}.${VER_REL}" \
"libofs_cpeng.so" \
"libopae-c++-nlb.so" \
"libopae-c++-nlb.so.${VER_MAJ}" \
"libopae-c++-nlb.so.${VER_MAJ}.${VER_MIN}.${VER_REL}" \
"libopae-c++-utils.so" \
"libopae-c++-utils.so.${VER_MAJ}" \
"libopae-c++-utils.so.${VER_MAJ}.${VER_MIN}.${VER_REL}" \
"libopae-c.so" \
"libopae-c.so.${VER_MAJ}" \
"libopae-c.so.${VER_MAJ}.${VER_MIN}.${VER_REL}" \
"libopae-cxx-core.so" \
"libopae-cxx-core.so.${VER_MAJ}" \
"libopae-cxx-core.so.${VER_MAJ}.${VER_MIN}.${VER_REL}" \
"libopaemem.so" \
"libopaemem.so.${VER_MAJ}" \
"libopaemem.so.${VER_MAJ}.${VER_MIN}.${VER_REL}" \
"libopaeuio.so" \
"libopaeuio.so.${VER_MAJ}" \
"libopaeuio.so.${VER_MAJ}.${VER_MIN}.${VER_REL}" \
"libopaevfio.so" \
"libopaevfio.so.${VER_MAJ}" \
"libopaevfio.so.${VER_MAJ}.${VER_MIN}.${VER_REL}" \
"opae/libboard_a10gx.so" \
"opae/libboard_d5005.so" \
"opae/libboard_n3000.so" \
"opae/libboard_n5010.so" \
"opae/libboard_n6010.so" \
"opae/libfpgad-vc.so" \
"opae/libfpgad-xfpga.so" \
"opae/libmodbmc.so" \
"opae/libopae-v.so" \
"opae/libxfpga.so"\
)

declare -ra LIB_DIRS_TO_CLEAN=(\
'/usr/local/lib' \
'/usr/local/lib64'\
)

declare -i DELETE=0
clean() {
  printf "$1\n"
  if [ ${DELETE} -eq 1 ]; then
    [ -f "$1" ] && rm -f "$1"
    [ -d "$1" ] && rm -rf "$1"
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
  for d in ${LIB_DIRS_TO_CLEAN[@]} ; do
    for l in ${LIBS[@]} ; do
      if [ -f "$d/$l" -o -L "$d/$l" ]; then
        clean "$d/$l"
      fi
    done
  done
}

declare -rai PYTHON_MAJ=(3)
declare -rai PYTHON_MIN=(0 1 2 3 4 5 6 7 8 9)
declare -rai PYTHON_REL=(0 1 2 3 4 5 6 7 8 9)

declare -ra PYTHON_DIRS_TO_CLEAN=(\
'/usr/lib' \
'/usr/local/lib' \
'/usr/local/lib64'\
)

python_files() {
  for d in ${PYTHON_DIRS_TO_CLEAN[@]} ; do

    for maj in ${PYTHON_MAJ[@]} ; do
      for min in ${PYTHON_MIN[@]} ; do
        py="$d/python${maj}.${min}"
        if [ -d "$py" ]; then
          py="$py/site-packages"
          if [ -d "$py" ]; then

            [ -d "$py/ethernet" ] && printf "%s\n" "$py/ethernet"

            files=($py/hssi_ethernet-*.egg-info)
            for f in ${files[@]} ; do
              printf "%s\n" "$f"
            done

            files=($py/libvfio*.so)
            for f in ${files[@]} ; do
              printf "%s\n" "$f"
            done

            [ -d "$py/opae" ] && printf "%s\n" "$py/opae"

            files=($py/opae.diag-*.egg-info)
            for f in ${files[@]} ; do
              printf "%s\n" "$f"
            done

            files=($py/opae.io-*.pth)
            for f in ${files[@]} ; do
              printf "%s\n" "$f"
            done

            files=($py/opae.io-*.egg-info)
            for f in ${files[@]} ; do
              printf "%s\n" "$f"
            done

            [ -d "$py/pacsign" ] && printf "%s\n" "$py/pacsign"

            files=($py/pacsign-*.egg-info)
            for f in ${files[@]} ; do
              printf "%s\n" "$f"
            done

            files=($py/pyopaeuio-*.egg-info)
            for f in ${files[@]} ; do
              printf "%s\n" "$f"
            done

            files=($py/pyopaeuio*.so)
            for f in ${files[@]} ; do
              printf "%s\n" "$f"
            done

          fi
        fi
      done 
    done

    for maj in ${PYTHON_MAJ[@]} ; do
      for min in ${PYTHON_MIN[@]} ; do
        for rel in ${PYTHON_REL[@]} ; do
          py="$d/python${maj}.${min}.${rel}"
          if [ -d "$py" ]; then
            py="$py/site-packages"
            if [ -d "$py" ]; then

              [ -d "$py/ethernet" ] && printf "%s\n" "$py/ethernet"

              files=($py/hssi_ethernet-*.egg-info)
              for f in ${files[@]} ; do
                printf "%s\n" "$f"
              done

              files=($py/libvfio*.so)
              for f in ${files[@]} ; do
                printf "%s\n" "$f"
              done

              [ -d "$py/opae" ] && printf "%s\n" "$py/opae"

              files=($py/opae.diag-*.egg-info)
              for f in ${files[@]} ; do
                printf "%s\n" "$f"
              done

              files=($py/opae.io-*.pth)
              for f in ${files[@]} ; do
                printf "%s\n" "$f"
              done

              files=($py/opae.io-*.egg-info)
              for f in ${files[@]} ; do
                printf "%s\n" "$f"
              done

              [ -d "$py/pacsign" ] && printf "%s\n" "$py/pacsign"

              files=($py/pacsign-*.egg-info)
              for f in ${files[@]} ; do
                printf "%s\n" "$f"
              done

              files=($py/pyopaeuio-*.egg-info)
              for f in ${files[@]} ; do
                printf "%s\n" "$f"
              done

              files=($py/pyopaeuio*.so)
              for f in ${files[@]} ; do
                printf "%s\n" "$f"
              done

            fi
          fi
        done
      done 
    done

  done
}

clean_python() {
  for f in `python_files` ; do
    clean "$f"
  done
}

show_help() {
  printf "Usage: clean [-b] [-l] [-p] [-h] [-c]\n"
  printf "\n"
  printf "  -b : clean OPAE binaries\n"
  printf "  -l : clean OPAE libraries\n"
  printf "  -p : clean OPAE Python artifacts\n"
  printf "  -h : display this help\n"
  printf "  -c : yes, really delete the files\n"
}

declare -i GOT_CMD=0

while getopts ":bclph" o; do
  case "$o" in
    b)
      clean_bins
      GOT_CMD=1
    ;;

    c)
      DELETE=1
    ;;

    l)
      clean_libs
      GOT_CMD=1
    ;;

    p)
      clean_python
      GOT_CMD=1
    ;;

    h)
      show_help
      exit 1
    ;;
  esac
done

if [ ${GOT_CMD} -eq 0 ]; then
  show_help
  exit 1
fi
