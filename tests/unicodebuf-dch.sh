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

DCHtest() {
  printf "1234567890abcdefgあいうえおかきくけこさしすせそ"
  Col $2
  CSI "$1P"
  printf "\n"
}

ret=0

InitScreen

for i in 1 2 3 4 5; do
  DCHtest $i 18
done

for i in 1 2 3 4 5; do
  DCHtest $i 21
done

cat <<_EoF_

=== 正しい出力は以下 ===

1234567890abcdefg いうえおかきくけこさしすせそ
1234567890abcdefgいうえおかきくけこさしすせそ
1234567890abcdefg うえおかきくけこさしすせそ
1234567890abcdefgうえおかきくけこさしすせそ
1234567890abcdefg えおかきくけこさしすせそ
1234567890abcdefgあ うえおかきくけこさしすせそ
1234567890abcdefgあ  えおかきくけこさしすせそ
1234567890abcdefgあ えおかきくけこさしすせそ
1234567890abcdefgあ  おかきくけこさしすせそ
1234567890abcdefgあ おかきくけこさしすせそ
_EoF_

for i in 3 2 1; do
  printf "%d 秒後に画面を再描画します\r" $i
  read -t 1 && ret=1 && break
done

CSI 7t		# 画面再描画

exit $ret
