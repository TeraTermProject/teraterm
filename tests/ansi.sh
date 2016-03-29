#!/bin/sh

printf "\\033[0m\n"
i=30
while [ $i -lt 38 ]; do
  j=40
  while [ $j -lt 48 ]; do
    printf "\\033[${i};${j}m Forward:%d Back:%d \\033[0m " $i $j
      for k in 1 4 5 7; do
        printf "\\033[${i};${j};${k}m decolate:%d \\033[0m " $k
        k=$(($k+1))
      done
    echo ""
    j=$(($j+1))
  done
  i=$(($i+1))
done
