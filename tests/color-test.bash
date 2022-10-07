#!/usr/local/bin/bash

pause() {
  printf "=>push return<= "
  read dummy
  echo
}

test_color() {
  case $1 in
    ?) len=25;;
    ??) len=26;;
    ???) len=27;;
    *) return;;
  esac
  
  echo -ne "\e]4;$1;?\a"
  IFS=: read -n $len dummy cstr
  reply=$(echo $cstr | tr -d '\07')
  h=$(printf "%1x0" $1)
  expected="$h$h/$h$h/$h$h"

  if [ $reply = $expected ]; then
    printf "o "
  else
    printf "x "
  fi
  printf "%d: %s %s\n" $1 $reply $expected
}

OLD_MODE=$(stty -g)
stty -echo

seq 0 15 | while read c; do
  printf "\033[48;5;${c}m  "
done
printf "\033[m\n"

pause

seq 0 15 | while read c; do
  printf "\033]4;${c};#%1x%1x%1x\033\\" $c $c $c
done

pause

for c in $(seq 0 15); do
  test_color $c
done

pause

seq 0 15 | while read c; do
  printf "\033]104;${c}\033\\"
done

stty $OLD_MODE
