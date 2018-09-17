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
    mcl = 1
    while (mcl <= 4) {
        set output "| ps2pdf - read_credit_mcl" . mcl . ".pdf"
        set title platform . " Uncached READ Varying Request Buffer Credits  (MCL=" . mcl . ")" offset 0,1 font ",18"
        set xrange [0:512]
        if (mcl > 1) {
            set xrange [0:1024]
        }

        plot 'stats/lat_mcl' . mcl . '_vc0.dat' index 2 using ($1):($2) with lines ls 1 title "VA Bandwidth", \
             'stats/lat_mcl' . mcl . '_vc0.dat' index 2 using ($1):(cycle_time * $10) axes x1y2 with lines ls 2 title "VA Latency", \
             'stats/lat_map_mcl' . mcl . '_vc0.dat' index 2 using ($1):($2) with lines ls 3 title "VC Map Bandwidth", \
             'stats/lat_map_mcl' . mcl . '_vc0.dat' index 2 using ($1):(cycle_time * $10) axes x1y2 with lines ls 4 title "VC Map Latency", \
             'stats/lat_ord_map_mcl' . mcl . '_vc0.dat' index 2 using ($1):($2) with lines ls 5 title "ROB VC Map Bandwidth", \
             'stats/lat_ord_map_mcl' . mcl . '_vc0.dat' index 2 using ($1):(cycle_time * $10) axes x1y2 with lines ls 6 title "ROB VC Map Latency"


        set output "| ps2pdf - write_credit_mcl" . mcl . ".pdf"
        set title platform . " Uncached WRITE Varying Request Buffer Credits  (MCL=" . mcl . ")" offset 0,1 font ",18"
        set xrange [0:256]

        plot 'stats/lat_mcl' . mcl . '_vc0.dat' index 5 using ($1):($3) with lines ls 1 title "VA Bandwidth", \
             'stats/lat_mcl' . mcl . '_vc0.dat' index 5 using ($1):(cycle_time * $12) axes x1y2 with lines ls 2 title "VA Latency", \
             'stats/lat_map_mcl' . mcl . '_vc0.dat' index 5 using ($1):($3) with lines ls 3 title "VC Map Bandwidth", \
             'stats/lat_map_mcl' . mcl . '_vc0.dat' index 5 using ($1):(cycle_time * $12) axes x1y2 with lines ls 4 title "VC Map Latency"

        mcl = mcl * 2
    }
}
else {
    set output "| ps2pdf - write_credit_vc" . channel_number . ".pdf"
    set title platform . " Uncached WRITE Varying Request Buffer Credits" offset 0,1 font ",18"
    set xrange [0:128]

    plot 'stats/lat_mcl1_vc' . channel_number . '.dat' index 5 using ($1):($3) with lines ls 1 title channel_name . " Bandwidth (MCL=1)", \
         'stats/lat_mcl1_vc' . channel_number . '.dat' index 5 using ($1):(cycle_time * $12) axes x1y2 with lines ls 2 title channel_name . " Latency (MCL=1)", \
         'stats/lat_mcl2_vc' . channel_number . '.dat' index 5 using ($1):($3) with lines ls 3 title channel_name . " Bandwidth (MCL=2)", \
         'stats/lat_mcl2_vc' . channel_number . '.dat' index 5 using ($1):(cycle_time * $12) axes x1y2 with lines ls 4 title channel_name . " Latency (MCL=2)", \
         'stats/lat_mcl4_vc' . channel_number . '.dat' index 5 using ($1):($3) with lines ls 5 title channel_name . " Bandwidth (MCL=4)", \
         'stats/lat_mcl4_vc' . channel_number . '.dat' index 5 using ($1):(cycle_time * $12) axes x1y2 with lines ls 6 title channel_name . " Latency (MCL=4)"
}


##
## Always plot these for the requested channel
##

set output "| ps2pdf - read_credit_vc" . channel_number . ".pdf"
set title platform . " Uncached READ Varying Request Buffer Credits" offset 0,1 font ",18"
if (channel_name eq "VA") {
    set xrange [0:512]
}
else {
    set xrange [0:256]
}

plot 'stats/lat_mcl1_vc' . channel_number . '.dat' index 2 using ($1):($2) with lines ls 1 title channel_name . " Bandwidth (MCL=1)", \
     'stats/lat_mcl1_vc' . channel_number . '.dat' index 2 using ($1):(cycle_time * $10) axes x1y2 with lines ls 2 title channel_name . " Latency (MCL=1)", \
     'stats/lat_mcl2_vc' . channel_number . '.dat' index 2 using ($1):($2) with lines ls 3 title channel_name . " Bandwidth (MCL=2)", \
     'stats/lat_mcl2_vc' . channel_number . '.dat' index 2 using ($1):(cycle_time * $10) axes x1y2 with lines ls 4 title channel_name . " Latency (MCL=2)", \
     'stats/lat_mcl4_vc' . channel_number . '.dat' index 2 using ($1):($2) with lines ls 5 title channel_name . " Bandwidth (MCL=4)", \
     'stats/lat_mcl4_vc' . channel_number . '.dat' index 2 using ($1):(cycle_time * $10) axes x1y2 with lines ls 6 title channel_name . " Latency (MCL=4)"

mcl = 1
while (mcl <= 4) {
    set output "| ps2pdf - read_credit_vc" . channel_number . "_rob_mcl" . mcl . ".pdf"
    set title platform . " Uncached READ Varying Request Buffer Credits  (MCL=" . mcl . ")" offset 0,1 font ",18"

    plot 'stats/lat_mcl' . mcl . '_vc' . channel_number . '.dat' index 2 using ($1):($2) with lines ls 1 title channel_name . " Bandwidth", \
         'stats/lat_mcl' . mcl . '_vc' . channel_number . '.dat' index 2 using ($1):(cycle_time * $10) axes x1y2 with lines ls 2 title channel_name . " Latency", \
         'stats/lat_ord_mcl' . mcl . '_vc' . channel_number . '.dat' index 2 using ($1):($2) with lines ls 3 title "ROB " . channel_name . " Bandwidth", \
         'stats/lat_ord_mcl' . mcl . '_vc' . channel_number . '.dat' index 2 using ($1):(cycle_time * $10) axes x1y2 with lines ls 4 title "ROB " . channel_name . " Latency"

    mcl = mcl * 2
}
