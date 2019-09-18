#!/bin/sh

CSI() {
  printf "\033[%s" "$1"
}

IRM() {
  if [ "$1" == "on" ]; then
    CSI 4h
  else
    CSI 4l
  fi
}

Col() {
  CSI "$1G"
}

InitScreen() {
  CSI "8;24;80t" # 端末サイズを 80x24 に変更
  CSI "2J"	# 画面消去
  CSI "1;1H"	# カーソルを画面左上に移動
}

IRMtest() {
  IRM off

  printf "[IRM ${1}]\n"

  printf "1234567890abcdefgかきくけこさしすせそ"
  IRM $1
  Col 6		# 6 の位置にカーソルを移動
  printf "あいうえお\n"
  IRM off

  printf "1234567890abcdefgかきくけこさしすせそ"
  IRM $1
  Col 21	# "き"の後半部分にカーソルを移動
  printf "1234567890\n"

  printf "1234567890abcdefgかきくけこさしすせそ"
  IRM $1
  Col 21	# "き"の後半部分にカーソルを移動
  printf "あいうえお\n"

  IRM off
  printf "\n"
}

ret=0

InitScreen

IRMtest on
IRMtest off

cat <<_EoF_
=== 正しい出力は以下 ===

[IRM on]
12345あいうえお67890abcdefgかきくけこさしすせそ
1234567890abcdefgか 1234567890 くけこさしすせそ
1234567890abcdefgか あいうえお くけこさしすせそ

[IRM off]
12345あいうえおfgかきくけこさしすせそ
1234567890abcdefgか 1234567890 すせそ
1234567890abcdefgか あいうえお すせそ

_EoF_

for i in 3 2 1; do
  printf "%d 秒後に画面を再描画します\r" $i
  read -t 1 && ret=$? && break
done

CSI 7t		# 画面再描画

exit $ret
