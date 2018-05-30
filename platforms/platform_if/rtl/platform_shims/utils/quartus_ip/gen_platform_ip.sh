#!/bin/sh

if [ -z "${1}" ]; then
    echo "Usage: ${0} <Path to Quartus release>"
    exit 1
fi

# Is the argument a path to a Quartus release tree?
QUARTUS_REL="${1}"
for d in quartus ip; do
    if [[ ! -d "${QUARTUS_REL}/${d}" ]]; then
        echo "${QUARTUS_REL} is not a Quartus release. (No '${QUARTUS_REL}/${d}' directory.)"
        exit 1
    fi
done


##
## Main logic files.  Change module names from "altera_..." to "platform_utils_..."
##
src_files="\
    ip/altera/sopc_builder_ip/altera_avalon_dc_fifo/altera_avalon_dc_fifo.v \
    ip/altera/merlin/altera_avalon_mm_bridge/altera_avalon_mm_bridge.v \
    ip/altera/merlin/altera_avalon_mm_clock_crossing_bridge/altera_avalon_mm_clock_crossing_bridge.v \
    ip/altera/sopc_builder_ip/altera_avalon_dc_fifo/altera_dcfifo_synchronizer_bundle.v \
    ip/altera/primitives/altera_std_synchronizer/altera_std_synchronizer_nocut.v"

for s in $src_files; do
    src="${QUARTUS_REL}/${s}"
    dst=`basename "${s}" | sed -e 's/^altera/platform_utils/'`
    echo "$dst"

    # The final replacement enables synchronous reset by default
    sed -e 's/ altera_std/ platform_utils_std/g' \
        -e 's/ altera_avalon/ platform_utils_avalon/g' \
        -e 's/ altera_dcfifo/ platform_utils_dcfifo/g' \
        -e 's/parameter SYNC_RESET \([ ]*\)= 0/parameter SYNC_RESET \1= 1/' \
        "${src}" > "${dst}"
done


##
## Timing constraints have similar transformations, but we can't rely on the space
## before module names.
##
# Avalon DC FIFO timing constraint
sed -e '4,$s/altera_/platform_utils_/g' \
    "${QUARTUS_REL}/ip/altera/sopc_builder_ip/altera_avalon_dc_fifo/altera_avalon_dc_fifo.sdc" > platform_utils_avalon_dc_fifo.sdc

# DC FIFO timing constraint
sed -e 's/^REPLACE/\n## Apply the constraints\napply_sdc_pre_dcfifo "platform_utils_dc_fifo"/' \
    "${QUARTUS_REL}/ip/altera/megafunctions/fifo/dcfifo.sdc" > platform_utils_dc_fifo.sdc


# *** No longer needed as of 18.0 ***
##
## Special case for platform_utils_avalon_mm_clock_crossing_bridge.v.  Add the unused
## parameters of platform_utils_avalon_dc_fifo() instances to reduce lint warnings
## in simulation.
##
#repl=`awk '{printf "%s\\\\n",$0}' <<EOF
#out_csr_writedata  (32'b0),
#
#        .out_startofpacket(),
#        .out_endofpacket(),
#        .out_empty(),
#        .out_error(),
#        .out_channel(),
#        .in_csr_readdata(),
#        .out_csr_readdata(),
#        .almost_full_valid(),
#        .almost_full_data(),
#        .almost_empty_valid(),
#        .almost_empty_data()
#EOF`
#
#sed -i -s "s/out_csr_writedata  (32'b0) */${repl}/" platform_utils_avalon_mm_clock_crossing_bridge.v


echo
echo "  *** The copyrights must still be changed to BSD/MIT licenses.  Do this by hand. ***"
echo
echo "  +++ platform_utils_avalon_mm_clock_crossing_bridge.v must be edited. See README. +++"
