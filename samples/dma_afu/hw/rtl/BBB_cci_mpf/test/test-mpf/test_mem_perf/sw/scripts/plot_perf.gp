if (! exists("title")) title = "SKX MCL=1 VC=0"
if (! exists("datafile")) datafile = "results.dat"
if (! exists("ofile")) ofile = ""

set term postscript color enhanced font "Helvetica" 17 butt dashed

set grid xtics nomxtics ytics nomytics noztics nomztics \
  nox2tics nomx2tics noy2tics nomy2tics nocbtics nomcbtics

set dgrid3d 32,16 gauss

set autoscale fix
set xrange [] noreverse nowriteback
set yrange [] noreverse nowriteback
set logscale x
set cbrange [0:20]
set format x '%.0s%cB'

set key font ",13" above box
set xlabel "Memory Footprint" font ",15" offset 0,0.75
set xtics  font ",11" offset 0,0.5
set ylabel "Stride (64 byte lines)" font ",15" offset 2,0
set ytics  font ",11" offset 0.5,0
set zlabel "GB/s" font ",15" offset 2
set ztics  font ",11" offset 0.5

set cbtics font ",11" offset -0.5,0

#unset colorbox
#set border 4095
#set view 102,51

set palette defined (0 0 0 0.5, 1 0 0 1, 2 0 0.5 1, 3 0 1 1, 4 0.5 1 0.5, 5 1 1 0, 6 1 0.5 0, 7 1 0 0, 8 0.5 0 0)
set palette maxcolors 40

set pm3d explicit flush center
set pm3d map

set output "| ps2pdf - read_bw_" . ofile . ".pdf"
set title title . " Read Bandwidth (GB/s)" font ",18" offset 1,0
splot datafile index 0 using 1:2:3 with pm3d notitle

set output "| ps2pdf - write_bw_" . ofile . ".pdf"
set title title . " Write Bandwidth (GB/s)" font ",18" offset 1,0
splot datafile index 1 using 1:2:4 with pm3d notitle

set output "| ps2pdf - rw_bw_" . ofile . ".pdf"
set title title . " Read+Write Bandwidth (GB/s)" font ",18" offset 1,0
splot datafile index 2 using 1:2:($3+$4) with pm3d notitle
