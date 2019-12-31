#set terminal png
#set output filename

#set terminal png
#set output filename

set key below
#set key font "Times-Roman ,9"
#set key spacing 0.9

set size 1.0,1.0
set multiplot title "DC Performance" #font "Times-Roman,11"
set tmargin 4
set autoscale y
set ytics font ",8"
set xtics font ",8"
set xlabel "time" font ",7"
###############################################################

set title "RLC TX Queue Size Variation in DC" #font "Times-Roman,10"
set logscale y
set size 0.5, 0.5
set ylabel "queue size (byte)" font ",7"
set origin 0.0,0.5
set grid
plot "rlcUmLowLatTx_queue_menb_ue1_bearer1.txt" using 1:2 title "LTE eNB RLC TX Queue Size", "rlcUmTx_queue_senb3_ue1_bearer1.txt" using 1:2 title "mmWave eNB RLC TX Queue Size"lt 3

###############################################################

#set format y "%5.2f"
unset logscale y
set title "RLC Path Throughput in DC" #font "Times-Roman,10"
set size 0.5,0.5
set origin 0.0,0.0
set ylabel "throughput (Mbyte)" font ",7"
set grid
set key
plot "rlc_Tput_menb_ue1_bearer_1.txt" using 1:2 title "LTE Path Throughput" with lines, "rlc_Tput_senb_ue1_bearer_1.txt" using 1:3  title "mmWave eNB Path Throughput" with lines lt 3,"tcp_throughput_ue1.txt" using 1:3 title "Total Thoughput" with lines lt 7 lw 3

#################################################

set title "TCP Cwnd Variation" #font "Times-Roman,10"
set size 0.5,0.5
set origin 0.5,0.0
set grid
set ylabel "Cwnd size (byte)" font ",7"
set key
plot "tcp_cwnd_ue1.txt" using 1:2 title "TCP Cwnd" 

#######################################

set title "TCP RTT Variation" font "Times-Roman,10"
set size 0.5,0.5
set origin 0.5, 0.5
set grid
set ylabel "RTT (s)" font ",7"
set key
plot "tcp_rtt_ue1.txt" using 1:2 title "TCP RTT"

pause 20.1

#reread


                                                            
