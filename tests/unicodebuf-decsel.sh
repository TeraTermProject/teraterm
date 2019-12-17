#!/bin/sh

CSI() {
  printf "\033[%s" "$1"
}

DECSCA() {
  case "$1" in
    off|0)	ch=0;;
    on|1)	ch=1;;
  esac
  CSI ${ch}\"q
}

Line() {
  CSI "$1;1H"
}

SetLine() {
  DECSCA off
  printf "********************"
  DECSCA on
  printf "########################################"
  DECSCA off
  printf "********************"
  CSI 40G
}

InitScreen() {
  CSI "8;24;80t" # 端末サイズを 80x24 に変更
  CSI "2J"	# 画面消去
  CSI "1;1H"	# カーソルを画面左上に移動
}

ICHtest() {
  SetLine
  CSI "?0K"
  printf "\n"

  SetLine
  CSI "?1K"
  printf "\n"

  SetLine
  CSI "?2K"
  printf "\n"
}

ret=0

InitScreen

ICHtest

cat <<_EoF_

=== 正しい出力は以下 ===

********************########################################                    
                    ########################################********************
                    ########################################                    

_EoF_
