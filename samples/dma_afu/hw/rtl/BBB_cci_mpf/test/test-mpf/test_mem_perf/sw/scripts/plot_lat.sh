#!/bin/sh

##
## This script expects that run_lat.sh has been completed in two configurations:
## one with a ROB enabled and one without, using:
##
##   scripts/run_lat.sh
##   scripts/run_lat.sh ord
##

platform="SKX Xeon+FPGA"
if [ -n "${1}" ]; then
    platform="${1}_"
fi

if [ -f stats/lat_mcl1_vc1.dat ]; then
    # Multi-channel system -- assume 3 channels
    gnuplot -e "platform='${platform}'" scripts/plot_buffer_credits.gp
    gnuplot -e "platform='${platform}'; channel_name='VL0'; channel_number=1" scripts/plot_buffer_credits.gp
    gnuplot -e "platform='${platform}'; channel_name='VH0'; channel_number=2" scripts/plot_buffer_credits.gp

    gnuplot -e "platform='${platform}'; channel_name='VA';  channel_number=0" scripts/plot_buffer_credits_rw.gp
    gnuplot -e "platform='${platform}'; channel_name='VL0'; channel_number=1" scripts/plot_buffer_credits_rw.gp
    gnuplot -e "platform='${platform}'; channel_name='VH0'; channel_number=2" scripts/plot_buffer_credits_rw.gp
else
    # Assume just PCIe
    gnuplot -e "platform='${platform}'; channel_name='VH0'; channel_number=0" scripts/plot_buffer_credits.gp
fi

# Crop whitespace
for fn in read_*.pdf write_*.pdf rw_*.pdf
do
    pdfcrop --margins 10 ${fn} crop_${fn} >/dev/null
    mv -f crop_${fn} ${fn}
done

# Bookmarks
cat >/tmp/pdfmark.$$ <<EOF
[/PageMode /UseOutlines /Page 1 /DOCVIEW pdfmark
[/Count 3 /Page 1 /Title (Read Bandwidth) /OUT pdfmark
[/Page 1 /Title (VA) /OUT pdfmark
[/Page 8 /Title (VL0) /OUT pdfmark
[/Page 12 /Title (VH0) /OUT pdfmark
[/Count 3 /Page 16 /Title (Write Bandwidth) /OUT pdfmark
[/Page 16 /Title (VA) /OUT pdfmark
[/Page 19 /Title (VL0) /OUT pdfmark
[/Page 20 /Title (VH0) /OUT pdfmark
[/Count 3 /Page 21 /Title (Read+Write Bandwidth) /OUT pdfmark
[/Page 21 /Title (VA) /OUT pdfmark
[/Page 33 /Title (VL0) /OUT pdfmark
[/Page 39 /Title (VH0) /OUT pdfmark
EOF

# Merge into a single PDF
gs -dBATCH -dNOPAUSE -q -sDEVICE=pdfwrite -dPDFSETTINGS=/prepress -dCompatibilityLevel=1.4 -dUseCIEColor -sOutputFile=bw-lat.pdf read_*.pdf write_*.pdf rw_*.pdf /tmp/pdfmark.$$
rm read_*.pdf write_*.pdf rw_*.pdf
rm /tmp/pdfmark.$$
