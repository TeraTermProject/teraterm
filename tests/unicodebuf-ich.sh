#!/bin/sh

CSI() {
  printf "\033[%s" "$1"
}

Col() {
  CSI "$1G"
}

InitScreen() {
  CSI "8;24;80t" # 端末サイズを 80x24 に変更
  CSI "2J"	# 画面消去
  CSI "1;1H"	# カーソルを画面左上に移動
}

ICHtest() {
  printf "1234567890abcdefgあいうえおかきくけこさしすせそ"
  Col $2
  CSI "$1@"
  printf "\n"
}

ret=0

InitScreen

for i in 1 2 3 4 5; do
  ICHtest $i 21
done

cat <<_EoF_

=== 正しい出力は以下 ===

1234567890abcdefgあ   うえおかきくけこさしすせそ
1234567890abcdefgあ    うえおかきくけこさしすせそ
1234567890abcdefgあ     うえおかきくけこさしすせそ
1234567890abcdefgあ      うえおかきくけこさしすせそ
1234567890abcdefgあ       うえおかきくけこさしすせそ

_EoF_

for i in 3 2 1; do
  printf "%d 秒後に画面を再描画します\r" $i
  read -t 1 && ret=1 && break
done

CSI 7t		# 画面再描画

exit $ret
