#!/bin/sh
#
# unicodebuf-combining1.sh: 結合文字列の表示テスト1
#
# さまざまな文字に濁点、および半濁点を付加して表示する。
# 表示のされ方はフォントによって変わってくる。
#
# また、結合文字が遅れて送られて来た場合のテストも兼ねる。
# 一行目は基底文字と結合文字がほぼ同時に送られるが、
# 二行目は基底文字と結合文字の間に1秒のウェイトが入る。
#

CombiningTest() {
	WAIT=1
	case $1 in
		(-w)	shift; WAIT=$1; shift;;
	esac

	while [ $# -gt 0 ]; do
		# 濁点
		printf "%s" "$1"
		[ $WAIT -gt 0 ] && sleep $WAIT
		printf "\343\202\231 "
		sleep 1

		# 半濁点
		printf "%s" "$1"
		[ $WAIT -gt 0 ] && sleep $WAIT
		printf "\343\202\232 "

		sleep 1

		shift
	done
	printf "\x1b[7t\n"	# refresh screen
}

#test_chars="か き く け こ ハ ヒ フ ヘ ホ あ い う え お A B C D E a b c d e"
test_chars="か き ハ ヒ あ い ヰ ゑ A B C a b c"

CombiningTest -w 0 $test_chars
CombiningTest -w 1 $test_chars

# vim: ts=8 sw=8 :
