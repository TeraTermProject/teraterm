#!/bin/sh

printf "\033[2J"

for i in $(seq 9); do
	printf "\033[${i};${i}Hあいうえおかきくけこ"
done

printf '\033[2*x\033[1;7;9;22;7$r\033[0*x\033[7t\n'
