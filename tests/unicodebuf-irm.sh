#!/bin/sh

CSI() {
  printf "\033[%s" "$1"
}

InitScreen() {
  CSI "8;24;80t" # 端末サイズを 80x24 に変更
  CSI "2J"	# 画面消去
  CSI "1;1H"	# カーソルを画面左上に移動
}

IRMtest() {
  printf "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyz1234567890"
  CSI 4h	# 挿入モード有効化
  CSI 37G	# 37桁目に移動
  printf "%*.*s\n" $1 $1 "*********************************"
  CSI 4l	# 挿入モード解除
}

ret=0

InitScreen

for i in 1 2 3 4 5 6 7 8 9 10; do
  IRMtest $i
done

cat <<_EoF_

=== 正しい出力は以下 ===

ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890*abcdefghijklmnopqrstuvwxyz1234567890
ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890**abcdefghijklmnopqrstuvwxyz1234567890
ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890***abcdefghijklmnopqrstuvwxyz1234567890
ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890****abcdefghijklmnopqrstuvwxyz1234567890
ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890*****abcdefghijklmnopqrstuvwxyz1234567890
ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890******abcdefghijklmnopqrstuvwxyz1234567890
ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890*******abcdefghijklmnopqrstuvwxyz1234567890
ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890********abcdefghijklmnopqrstuvwxyz1234567890
ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890*********abcdefghijklmnopqrstuvwxyz123456789
ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890**********abcdefghijklmnopqrstuvwxyz12345678
_EoF_

for i in 3 2 1; do
  printf "%d 秒後に画面を再描画します\r" $i
  read -t 1 && ret=1 && break
done

CSI 7t		# 画面再描画

exit $ret
