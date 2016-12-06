#!/bin/bash

if [[ $# != 1 ]] ; then printf "\nError: protocol name expected as a parameter\n\n" ; exit 1; fi

declare -a N1
declare -a N2
readonly ProtocolName=$1

rm -f ../data/${ProtocolName}_average.dat ../data/${ProtocolName}_average.png

fname=../data/${ProtocolName}_throughput.dat

if [[ ! -r $fname ]]; then
	echo "Error reading ${ProtocolName}_throughput.dat"
	exit 1
fi

N1=( $(head -n 1 $fname) )
N2=( $(tail -n 1 $fname) )

declare D1
declare D2

D1=$(bc <<< "scale=9;${N1[0]}/${N1[2]}")
D2=$(bc <<< "scale=9;${N2[0]}/${N2[2]}")


declare B
declare B_NUM
declare B_DEN

B_NUM=$(bc <<< "scale=9;${N2[0]}-${N1[0]}")
B_DEN=$(bc <<< "scale=9;${D2}-${D1}")
B=$(bc <<< "scale=9;${B_NUM}/${B_DEN}")

declare L
declare L_NUM
declare L_DEN

L_NUM=$(bc <<< "scale=9;(${D1}*${N2[0]})-(${D2}*${N1[0]})")
L_DEN=$(bc <<< "scale=9;${N2}-${N1}")
L=$(bc <<< "scale=9;${L_NUM}/${L_DEN}")

gnuplot <<-eNDgNUPLOTcOMMAND
	set term png size 900, 700
	set output "../data/${ProtocolName}_average.png"
	set logscale x 2
	set logscale y 10
	set xlabel "msg size (B)"
	set ylabel "throughput (KB/s)"
	lbf(x) = x / ( $L + x / $B )
	plot "../data/udp_throughput.dat" using 1:3 title "${ProtocolName} ping-pong Throughput" \
		with linespoints, lbf(x) title "Latency-Bandwidth model with L=$L and B=$B" \
		with linespoints
	clear
eNDgNUPLOTcOMMAND



