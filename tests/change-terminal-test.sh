#!/usr/bin/env bash

stty -echo

echo -ne '\e[18t'

IFS='[' read -t 1 -d t esc saved_size

echo -ne '\e[8;30;25t'
echo -ne '\e[2$~'
echo -ne '\e[1$}'

echo -ne hoge

echo -ne '\e[8;30;25t'
echo -ne '\e[0$}'

sleep 1

echo -ne '\e[25H'
echo -ne "#\n#\n#\n#\n#\n#\n#"

sleep 1

echo -ne '\e[6n'
IFS='[' read -t 1 -d R esc pos

echo -ne '\e[0$~'
echo -ne "\e[${saved_size}t"

echo -ne '\e[H\e[2J'

if [ "30;2" = "$pos" ]; then
	echo "OK ($pos)"
else
	echo "NG ($pos)"
fi

stty echo
