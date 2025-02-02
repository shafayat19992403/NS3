# #!/bin/bash
# n=$1
# folder=scratch/tcphs 
# dataFile=$folder/data.txt
# mkdir -p "$folder/HighSpeed"
# mkdir -p "$folder/AdaptiveReno"
# rm -f $dataFile
# touch $dataFile

# bottleneck_rate_array=()
# for i in {0..n}; do
#   bottleneck_rate_array+=( "$((2 ** $i))" )
# done

# init=2
# packetloss_exp_array=()
# for i in {0..n}; do
#   packetloss_exp_array+=( "$(bc <<< "$init + $i*0.5")" )
# done

# echo "bottleneck_rate_array: ${bottleneck_rate_array[@]}"
# echo "packetloss_exp_array: ${packetloss_exp_array[@]}"

#!/bin/bash

#!/bin/bash

n=$1
runfile=offline.cc
folder=scratch/tcphs
dataFile=$folder/data.txt
mkdir -p "$folder/HighSpeed"
mkdir -p "$folder/AdaptiveReno"
rm -f "$dataFile"
touch "$dataFile"
bottleneck_rate=50
packetloss_exp=6


bottleneck_rate_array=()
for i in $(seq 0 $((2 * n))); do
  bottleneck_rate_array+=( "$((2 ** $i))" )
done

init=2
packetloss_exp_array=()
for i in $(seq 0 $n); do
  packetloss_exp_array+=( "$(bc <<< "$init + $i*0.5")" )
done

# echo "bottleneck_rate_array: ${bottleneck_rate_array[@]}"
# echo "packetloss_exp_array: ${packetloss_exp_array[@]}"


# parameters - title, xlabel, ylabel, input file1, output file, xcolumn, y column1, y tick1, input file2, y column2, y tick2
plot_graph () {
gnuCommand="set terminal pngcairo size 1024,768 enhanced font 'Verdana,12';
set output '$5.png';
set title '$1' font 'Verdana,16';
set xlabel '$2' font 'Verdana,14';
set ylabel '$3' font 'Verdana,14';
plot '$4' using $6:$7 title '$8' with linespoints linestyle 1"


if [[ $# -gt 8 ]]
then
gnuCommand=$gnuCommand", '$9' using $6:${10} title '${11}' with
linespoints linestyle 2;"
else
gnuCommand=$gnuCommand";"
fi

if [[ $# -gt 11 ]]
then
gnuCommand=${gnuCommand//linespoints/lines}
fi

gnuCommand=$gnuCommand" exit;"

echo $gnuCommand | gnuplot
}

tcpTypeTwo="ns3::TcpHighSpeed"
for item in "${bottleneck_rate_array[@]}"; do
    ./ns3 run "scratch/$runfile --tcp2=$tcpTypeTwo --bttlnkRate=$item --exp=$packetloss_exp" 
done

plot_graph "Throughput vs Bottleneck Rate" "Bottleneck Rate" "Throughput" "$folder/data.txt" "$folder/HighSpeed/throughput_vs_bottleneckRate" 1 3 "TcpNewReno" "$folder/data.txt" 4 "TcpHighSpeed"
#plot_graph "Fairness Index vs Bottleneck Rate" "Bottleneck Rate" "Fairness Index" "$folder/data.txt" "$folder/HighSpeed/fairness_index_vs_bottleneckRate" 1 5 "Fairness Index"

rm -f $dataFile
touch $dataFile

for item in "${packetloss_exp_array[@]}"; do
    ./ns3 run "scratch/$runfile --tcp2=$tcpTypeTwo --bttlnkRate=$bottleneck_rate --exp=$item" 
done

plot_graph "Throughput vs Packetloss Ratio" "Packetloss Ratio" "Throughput" "$folder/data.txt" "$folder/HighSpeed/throughput_vs_packetloss_ratio" 2 3 "TcpNewReno" "$folder/data.txt" 4 "TcpHighSpeed"
#plot_graph "Fairness Index vs Packetloss Ratio" "Packetloss Ratio" "Fairness Index" "scratch/Offline2/data.txt" "scratch/Offline2/NewRenoVSHighSpeed/fairness_index_vs_packetloss_ratio" 2 5 "Fairness Index"

./ns3 run "scratch/$runfile --tcp2=$tcpTypeTwo --bttlnkRate=$bottleneck_rate --exp=$packetloss_exp" 

plot_graph "Congestion Window vs Time" "Time" "Congestion Window" "$folder/flow1.cwnd" "$folder/HighSpeed/Congestion_window_vs_time" 1 2 "TcpNewReno" "$folder/flow2.cwnd" 2 "TcpHighSpeed"

tcpTypeTwo="ns3::TcpAdaptiveReno"
rm -f $dataFile
touch $dataFile

tcpTypeTwo="ns3::TcpHighSpeed"
for item in "${bottleneck_rate_array[@]}"; do
    ./ns3 run "scratch/$runfile --tcp2=$tcpTypeTwo --bttlnkRate=$item --exp=$packetloss_exp" 
done

plot_graph "Throughput vs Bottleneck Rate" "Bottleneck Rate" "Throughput" "$folder/data.txt" "$folder/HighSpeed/throughput_vs_bottleneckRate" 1 3 "TcpNewReno" "$folder/data.txt" 4 "TcpHighSpeed"
#plot_graph "Fairness Index vs Bottleneck Rate" "Bottleneck Rate" "Fairness Index" "$folder/data.txt" "$folder/HighSpeed/fairness_index_vs_bottleneckRate" 1 5 "Fairness Index"

rm -f $dataFile
touch $dataFile

for item in "${packetloss_exp_array[@]}"; do
    ./ns3 run "scratch/$runfile --tcp2=$tcpTypeTwo --bttlnkRate=$bottleneck_rate --exp=$item" 
done

plot_graph "Throughput vs Packetloss Ratio" "Packetloss Ratio" "Throughput" "$folder/data.txt" "$folder/HighSpeed/throughput_vs_packetloss_ratio" 2 3 "TcpNewReno" "$folder/data.txt" 4 "TcpHighSpeed"
#plot_graph "Fairness Index vs Packetloss Ratio" "Packetloss Ratio" "Fairness Index" "scratch/Offline2/data.txt" "scratch/Offline2/NewRenoVSHighSpeed/fairness_index_vs_packetloss_ratio" 2 5 "Fairness Index"

./ns3 run "scratch/$runfile --tcp2=$tcpTypeTwo --bttlnkRate=$bottleneck_rate --exp=$packetloss_exp" 

plot_graph "Congestion Window vs Time" "Time" "Congestion Window" "$folder/flow1.cwnd" "$folder/HighSpeed/Congestion_window_vs_time" 1 2 "TcpNewReno" "$folder/flow2.cwnd" 2 "TcpHighSpeed"

