#!/bin/sh

CSI() {
  printf "\033[%s" "$1"
}

InitScreen() {
  CSI "8;24;20t" # 端末サイズを 20x24 に変更
  CSI "2J"	# 画面消去
  CSI "1;1H"	# カーソルを画面左上に移動
}

InitScreen

# 文字色を赤(色番号1)にする
# 出力 "12345678901234567890"
# 出力 "123456789012345678あ"
# 文字属性を解除(色を戻す)
# 挿入モード設定
# 左へ5
# 出力 "a"
# 挿入モード解除

# 半角1文字
printf "test 1\n"
printf "\e[31m12345678901234567890\e[m\e[4h\e[5Da\e[4l\n"
printf "expect\n"
printf "\e[31m12345678901234\e[ma\e[31m56789\e[m\n"

# 全角1文字
printf "test 2\n"
printf "\e[31m12345678901234567890\e[m\e[4h\e[5Dあ\e[4l\n"
printf "expect\n"
printf "\e[31m12345678901234\e[mあ\e[31m5678\e[m\n"

# 行末に全角
printf "test 3\n"
printf "\e[31m123456789012345678あ\e[m\e[4h\e[5Da\e[4l\n"
printf "expect\n"
printf "\e[31m12345678901234\e[ma\e[31m5678 \e[m\n"
