#!/bin/sh
#
# DECSACE の動作テスト
# 通常は S と s に色および下線が付く。
# DECSACE:2 の時のみ S だけに色と下線が付く。
#

waitkey() {
	printf "push return"
	read pause
}

SetTestingScreen() {
	printf '\033[2J\033[18H'
	printf '\033[46;1;1;16;9999$x'

	printf '\033[4;1;4;9999$z'
	printf '\033[9;1;9;9999$z'

	printf '\033[83;2;11;2;20$x'
	printf '\033[83;6;11;7;20$x'
	printf '\033[83;11;11;15;20$x'

	printf '\033[115;6;21;6;9999$x'
	printf '\033[115;7;1;7;10$x'

	printf '\033[115;11;21;14;9999$x'
	printf '\033[115;12;1;15;10$x'
}

TestDECSACE() {
	SetTestingScreen

	[ $# -gt 0 ] && printf "\033[${1}*x"

	printf '\033[2;11;2;20;1;35$r'
	printf '\033[2;11;2;20;4$t'

	printf '\033[6;11;7;20;1;35$r'
	printf '\033[6;11;7;20;4$t'

	printf '\033[11;11;15;20;1;35$r'
	printf '\033[11;11;15;20;4$t'

	if [ "x$1" = "x" ]; then
		echo "Default"
	else
		echo "DECSACE: ${1}"
	fi

	sleep 1

	printf "\033[7t"
}

TestDECSACE
waitkey

TestDECSACE 0
waitkey

TestDECSACE 1
waitkey

TestDECSACE 2
waitkey

TestDECSACE ""
