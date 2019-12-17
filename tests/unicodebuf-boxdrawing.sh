#!/bin/sh

# https://www.teraterm.jp/manual/4.59/html/setup/teraterm-term.html#UnicodeDecSpMapping
# https://en.wikipedia.org/wiki/Box-drawing_character

CSI() {
  printf "\033[%s" "$1"
}

Col() {
  CSI "$1G"
}

Loc() {
  CSI "$2;$1H"
}

InitScreen() {
  CSI "8;24;80t" # 端末サイズを 80x24 に変更
  CSI "2J"	# 画面消去
  CSI "1;1H"	# カーソルを画面左上に移動
}

InitScreen

printf "\e(0"
printf "lqqqqqqqqqqqqqqqqqqqqk\n"
printf "x                    x\n"
printf "x                    x\n"
printf "mqqqqqqqqqqqqqqqqqqqqj\n"
printf "\e(B"

Loc 1 6
printf "\e[44m"
printf "ああああああああああああああああ\n"
printf "Aああああああああああああああああ\n"
printf "ああああああああああああああああ\n"
printf "Aああああああああああああああああ\n"
printf "あああああああああああああああああ\n"
printf "Aああああああああああああああああ\n"

printf "\e(0"
printf "\e[41m"
Loc 3 7
printf "lqqqqqqqqqqqqqqqqqqqqk"
Loc 3 8
printf "x                    x"
Loc 3 9
printf "x                    x"
Loc 3 10
printf "mqqqqqqqqqqqqqqqqqqqqj"
printf "\e(B\n\n\n"
printf "\e[40m"

for i in 3 2 1; do
  printf "%d 秒後に画面を再描画します\r" $i
  read -t 1 && ret=$? && break
done

CSI 7t		# 画面再描画
