#!/bin/sh

CSI() {
  printf "\033[%s" "$1"
}

InitScreen() {
  CSI "8;24;80t" # 端末サイズを 80x24 に変更
  CSI "2J"	# 画面消去
  CSI "1;1H"	# カーソルを画面左上に移動
}

# 画面再描画
Redraw() {
	CSI 7t
}

Left() {
	printf "\e[%sD" "$1"
}

Wait_3sec() {
	for i in 3 2 1; do
		printf "wait %d 秒\r" $i
		read -t 1 && ret=1 && break
	done
}

simple() {
	printf "https://ttssh2.osdn.jp/"
}

Wrap() {
	InitScreen
	printf "wrap test\n"
	for ((i=80-9; i <= 80; i++)); do
		for ((j=0; j<i; j++)); do
			printf "_"
		done
		printf "https://ttssh2.osdn.jp/"
		printf "\n"
	done
}

Repeat() {
	InitScreen
	printf "repeat test\n"
	count=50
	for ((i=0; i < $count; i++)); do
		printf "https://ttssh2.osdn.jp/  "
	done
	printf "\n"
}

Break() {
	InitScreen
	printf "break url test\n"
	for ((i=14; i < 23; i++)); do
		printf "https://ttssh2.osdn.jp/"
		Left "$i"
		printf "!\n"
	done
}

InitScreen
Break
Wait_3sec
Redraw
Wait_3sec

Wrap
Wait_3sec
Redraw
Wait_3sec

Repeat
Wait_3sec
Redraw
Wait_3sec

