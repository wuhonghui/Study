#!/bin/sh

for i in `seq 32`; do
	echo -n -e "$i \E[${i}m str\E[0m\t"
	echo "\E[${i}mstr\E[0m"
done
