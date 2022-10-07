#!/bin/sh

pause() {
  printf "==> push return <== "
  read dummy
}

CSI() {
  while [ $# -gt 0 ]; do
    printf "\033[%s" $1
    shift
  done
}

SGR() {
  CSI ""
  printf "%s" $1
  shift
  while [ $# -gt 0 ]; do
    printf ";%s" $1
    shift
  done

  printf m
}

bcetest() {
  CSI 2J H

  SGR 31 46; printf "EL"; CSI K; SGR 0; echo
  SGR 31 46; printf "ICH"; CSI 999@; SGR 0; echo
  SGR 31 46; printf "DCH"; CSI 999P; SGR 0; echo
  SGR 31 46; printf "ECH"; CSI 999X; SGR 0; echo
  SGR 31 46; CSI L; printf "IL"; SGR 0; echo
  SGR 31 46 7; printf "EL+SGR7"; CSI K; SGR 0; echo
  SGR 31 46; printf "ED"; CSI J; SGR 0; echo; echo; echo

  printf "EL"; CSI K; SGR 0; echo
  SGR 7; printf "EL+SGR7"; CSI K; SGR 0; echo
  printf "ED"; CSI J; SGR 0; echo; echo; echo
}

bcetest

pause

CSI "?5h"

bcetest

pause

CSI "?5l"
CSI 9999H
