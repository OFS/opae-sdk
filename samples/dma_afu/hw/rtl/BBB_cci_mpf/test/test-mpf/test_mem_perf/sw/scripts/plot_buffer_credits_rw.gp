if (! exists("platform")) platform = "SKX Xeon+FPGA"

if (! exists("channel_name")) channel_name = "VA"
if (! exists("channel_number")) channel_number = "0"

# Cycle time in ns
cycle_time=2.5

set term postscript color enhanced font "Helvetica" 17 butt dashed

set ylabel "Bandwidth (GB/s)" offset 1,0 font ",15"
set y2label "Latency (ns)" offset -1.75,0 font ",15"
set xlabel "Maximum Outstanding Lines" font ",15"

set mxtics 3
set boxwidth 0.8
set xtics out font ",12"

set ytics out nomirror font ",12"
set y2tics out font ",12"

set yrange [0:]
set y2range [0:]
#set size square
set bmargin 0.5
set tmargin 0.5
set lmargin 3.0
set rmargin 6.25

set key on inside bottom right width 2 samplen 4 spacing 1.5 font ",14"
set style fill pattern
set style data histograms

set style line 1 lc rgb "red" lw 3
set style line 2 lc rgb "red" lw 3 dashtype "-"
set style line 3 lc rgb "blue" lw 3
set style line 4 lc rgb "blue" lw 3 dashtype "-"
set style line 5 lc rgb "green" lw 3
set style line 6 lc rgb "green" lw 3 dashtype "-"

if (channel_name eq "VA") {
    set xrange [0:512]
}
else {
    set xrange [0:256]
}

mcl = 1
while (mcl <= 4) {
    set output "| ps2pdf - rw_credit_vc" . channel_number . "_mcl" . mcl . ".pdf"
    set title platform . " Uncached RD+WR Varying Request Buffer Credits  (MCL=" . mcl . ")" offset 0,1 font ",18"

    plot 'stats/lat_mcl' . mcl . '_vc' . channel_number . '.dat' index 8 using ($1):($2) with lines ls 1 title channel_name . " Read Bandwidth", \
         'stats/lat_mcl' . mcl . '_vc' . channel_number . '.dat' index 8 using ($1):(cycle_time * $10) axes x1y2 with lines ls 2 title channel_name . " Read Latency", \
         'stats/lat_mcl' . mcl . '_vc' . channel_number . '.dat' index 8 using ($1):($3) with lines ls 3 title channel_name . " Write Bandwidth", \
         'stats/lat_mcl' . mcl . '_vc' . channel_number . '.dat' index 8 using ($1):(cycle_time * $12) axes x1y2 with lines ls 4 title channel_name . " Write Latency", \
         'stats/lat_mcl' . mcl . '_vc' . channel_number . '.dat' index 8 using ($1):($2 + $3) with lines ls 5 title channel_name . " Total Bandwidth"

    set output "| ps2pdf - rw_credit_vc" . channel_number . "_mcl" . mcl . "_rob.pdf"
    set title platform . " Uncached RD+WR Varying Request Buffer Credits  (MCL=" . mcl . ")" offset 0,1 font ",18"

    plot 'stats/lat_ord_mcl' . mcl . '_vc' . channel_number . '.dat' index 8 using ($1):($2) with lines ls 1 title "ROB " . channel_name . " Read Bandwidth", \
         'stats/lat_ord_mcl' . mcl . '_vc' . channel_number . '.dat' index 8 using ($1):(cycle_time * $10) axes x1y2 with lines ls 2 title "ROB " . channel_name . " Read Latency", \
         'stats/lat_ord_mcl' . mcl . '_vc' . channel_number . '.dat' index 8 using ($1):($3) with lines ls 3 title channel_name . " Write Bandwidth", \
         'stats/lat_ord_mcl' . mcl . '_vc' . channel_number . '.dat' index 8 using ($1):(cycle_time * $12) axes x1y2 with lines ls 4 title channel_name . " Write Latency", \
         'stats/lat_ord_mcl' . mcl . '_vc' . channel_number . '.dat' index 8 using ($1):($2 + $3) with lines ls 5 title channel_name . " Total Bandwidth"


    if (channel_name eq "VA") {
        set output "| ps2pdf - rw_credit_vc" . channel_number . "_mcl" . mcl . "_xmap.pdf"
        set title platform . " Uncached RD+WR Varying Request Buffer Credits  (MCL=" . mcl . ")" offset 0,1 font ",18"

        plot 'stats/lat_map_mcl' . mcl . '_vc' . channel_number . '.dat' index 8 using ($1):($2) with lines ls 1 title "VC Map Read Bandwidth", \
             'stats/lat_map_mcl' . mcl . '_vc' . channel_number . '.dat' index 8 using ($1):(cycle_time * $10) axes x1y2 with lines ls 2 title "VC Map Read Latency", \
             'stats/lat_map_mcl' . mcl . '_vc' . channel_number . '.dat' index 8 using ($1):($3) with lines ls 3 title "VC Map Write Bandwidth", \
             'stats/lat_map_mcl' . mcl . '_vc' . channel_number . '.dat' index 8 using ($1):(cycle_time * $12) axes x1y2 with lines ls 4 title "VC Map Write Latency", \
             'stats/lat_map_mcl' . mcl . '_vc' . channel_number . '.dat' index 8 using ($1):($2 + $3) with lines ls 5 title "VC Map Total Bandwidth"

        set output "| ps2pdf - rw_credit_vc" . channel_number . "_mcl" . mcl . "_xmap_rob.pdf"
        set title platform . " Uncached RD+WR Varying Request Buffer Credits  (MCL=" . mcl . ")" offset 0,1 font ",18"

        plot 'stats/lat_ord_map_mcl' . mcl . '_vc' . channel_number . '.dat' index 8 using ($1):($2) with lines ls 1 title "ROB VC Map Read Bandwidth", \
             'stats/lat_ord_map_mcl' . mcl . '_vc' . channel_number . '.dat' index 8 using ($1):(cycle_time * $10) axes x1y2 with lines ls 2 title "ROB VC Map Read Latency", \
             'stats/lat_ord_map_mcl' . mcl . '_vc' . channel_number . '.dat' index 8 using ($1):($3) with lines ls 3 title "VC Map Write Bandwidth", \
             'stats/lat_ord_map_mcl' . mcl . '_vc' . channel_number . '.dat' index 8 using ($1):(cycle_time * $12) axes x1y2 with lines ls 4 title "VC Map Write Latency", \
             'stats/lat_ord_map_mcl' . mcl . '_vc' . channel_number . '.dat' index 8 using ($1):($2 + $3) with lines ls 5 title "VC Map Total Bandwidth"
    }

    mcl = mcl * 2
}
