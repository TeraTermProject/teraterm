#!/bin/sh
#
# DECCARA / DECRARA 対象領域確認スクリプト
# 同じ数字は同じ色 or 属性となるのが正しい。
# 画面の再描画前後で表示が変わらないのが正しい。
#

printf "\033[*2x\033[2J"

seqs=$(seq 9)

for i in $seqs; do
	xend=$((21 - $i))
	yend=$((20 - $i))
	char=$((47 + $i))

	printf '\033[%d;%d;%d;%d;%d$x' $char $i $i $yend $xend
done

for i in $seqs; do
	xend=$((21 - $i))
	yend=$((20 - $i))
	color=$((40 + $i % 8))
	printf '\033[%d;%d;%d;%d;%d$r' $i $i $yend $xend $color
done


for i in $seqs; do
	xstart=$(($i + 40))
	xend=$((61 - $i))
	yend=$((20 - $i))
	char=$((47 + $i))

	printf '\033[%d;%d;%d;%d;%d$x' $char $i $xstart $yend $xend
done

for i in $seqs; do
	xstart=$(($i + 40))
	xend=$((61 - $i))
	yend=$((20 - $i))

	case $i in
	1)	attr="1;7";;
	2)	attr="1;4;7";;
	3)	attr="4;5;7";;
	4)	attr="1;5;7";;
	5)	attr="1;4;7";;
	6)	attr="4;5;7";;
	7)	attr="1;5;7";;
	8)	attr="1;4;7";;
	9)	attr="4;5;7";;
	esac

	color=$((40 + $i % 8))
	printf '\033[%d;%d;%d;%d;%s$t' $i $xstart $yend $xend $attr
done

printf "\033[20;7HDECCARA\e[47GDECRARA\n\033[0*x"

sleep 1

# 画面再描画
printf "\033[7t"
