#!/bin/sh

trap "OSC 111; CSI m; exit" 0 2

ESC() {
  while [ $# -gt 0 ]; do
    printf "\033$1"
    shift
  done
}

CSI() {
  while [ $# -gt 0 ]; do
    ESC "[$1"
    shift
  done
}

OSC() {
  while [ $# -gt 0 ]; do
    ESC "]$1" '\'
    shift
  done
}

OSC "11;#440000"
CSI "4;37;44m" H 2J

echo "1234567890"; sleep 1

printf "shift to the right 5 columns"; sleep 1
CSI G
ESC 6 6 6 6 6
echo ""

sleep 2

CSI 6G
printf "shift to the left 3 columns"; sleep 1
CSI 999G
ESC 9 9 9
echo

sleep 2
