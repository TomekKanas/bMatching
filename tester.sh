#!/bin/bash

THREADS=(1 2 3 4 6)
#THREADS=(1)

if [ $# != 3 ] 
then
	echo "Usage : $0 program test b_limit"
	exit
fi

out=""
for i in ${THREADS[*]}
do
	out="$out$i " 
done

out="$out\n"

for i in ${THREADS[*]}
do
	if [ $i == ${THREADS[0]} ]
	then
		d1=$(date +%s.%N)
		./$1 $i $2 $3 > result.out
		d2=$(date +%s.%N)
	else
		d1=$(date +%s.%N)
		./$1 $i $2 $3 > result1.out
		d2=$(date +%s.%N)
		#echo "kupa" > result1.out
		if ! cmp -s result.out result1.out
		then
			echo "Results differ with thread numbers: ${THREADS[0]} $i"
			echo "First result:"
			cat result.out
			echo -e "Second result:"
			cat result1.out
			exit
		fi
	fi
	out="$out$(echo "$d2 - $d1" | bc) "
	echo "Progress: $i/${#THREADS[@]}";
done

#rm result1.out
echo -ne $out | column -t

