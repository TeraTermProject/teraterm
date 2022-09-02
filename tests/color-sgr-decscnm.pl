#!/usr/bin/perl

local $| = 1;

print "\x1b[0m";
print "SGR(Select Graphic Rendition) test\n";

if (1) {
	print "==================== support attributes\n";

	print "\x1b[0m";
	print "Normal            / SGR 0";
	print "\x1b[0m";
	print "\n";

	print "\x1b[39m";
	print "\x1b[49m";
	print "Normal            / SGR 39, 49";
	print "\x1b[0m";
	print "\n";

	print "\x1b[1m";
	print "Bold              / SGR 1";
	print "\x1b[0m";
	print "\n";

	print "\x1b[4m";
	print "Underline         / SGR 4";
	print "\x1b[0m";
	print "\n";

	print "\x1b[5m";
	print "Blink(Slow blink) / SGR 5";
	print "\x1b[0m";
	print "\n";

	print "\x1b[7m";
	print "Reverse           / SGR 7";
	print "\x1b[0m";
	print "\n";

	print "\x1b[0m";
	print "https://ttssh2.osdn.jp/  SGR 0(Normal) + URL string\n";

	print "\x1b[0m\x1b[31m";
	print "RED               / SGR 0 + SGR 31(FG Red)";
	print "\x1b[0m";
	print "\n";

	print "\x1b[0m\x1b[42m";
	print "RED               / SGR 0 + SGR 42(BG Green)";
	print "\x1b[0m";
	print "\n";

	print "\x1b[31m\x1b[42m";
	print "RED               / SGR 31(FG Red) + SGR 42(BG Green)";
	print "\x1b[0m";
	print "\n";

	print "\x1b[7m\x1b[31m\x1b[42m";
	print "Reverse           / SGR 7 + SGR 31(FG Red) + SGR 42(BG Green)";
	print "\x1b[0m";
	print "\n";

	print "\x1b[1m\x1b[41m";
	print "BOLD + BG RED     / SGR 1 + SGR 41(BG Red)";
	print "\x1b[0m";
	print "\n";

	print "\x1b[5m\x1b[46m";
	print "Blink(Slow blink) / SGR 5 + SGR 46(BG Cyan)";
	print "\x1b[0m";
	print "\n";

	print "\x1b[5m\x1b[106m";
	print "Blink(Slow blink) / SGR 5 + SGR 106(BG Bright Cyan)";
	print "\x1b[0m";
	print "\n";

	print "\x1b[4m\x1b[44m";
	print "Underline         / SGR 4 + SGR 44(BG Blue)";
	print "\x1b[0m";
	print "\n";

	print "\x1b[4m\x1b[104m";
	print "Underline         / SGR 4 + SGR 104(BG Bright Blue)";
	print "\x1b[0m";
	print "\n";

	print "\x1b[30m\x1b[40m";
	print "Underline         / SGR 30 + SGR 40 (FG&BG Black)";
	print "\x1b[0m";
	print "\n";

	print "\x1b[37m\x1b[47m";
	print "Underline         / SGR 37 + SGR 47 (FG&BG White(8color)/Gray(16,256color))";
	print "\x1b[0m";
	print "\n";

	print "\x1b[97m\x1b[107m";
	print "Underline         / SGR 97 + SGR 107 (FG&BG White(16,256color))";
	print "\x1b[0m";
	print "\n";
}

if (1) {
	print "==================== ANSI color\n";

	print "\x1b[0m";

	print "3bit(8) Standard color / 4bit(16) Darker color\n";
	print " FG: SGR 30..37 m  BG: SGR 40..47 m\n";
	for ($f = 0; $f < 8; $f++) {
		for ($b = 0; $b < 8; $b++) {
			printf("\x1b[%d;%dm %3d/%3d ", $f + 30, $b + 40, $f + 30, $b + 40);
		}
		print "\x1b[0m";
		print "\n";
	}
	print "\x1b[0m";
	print "\n";

	if (1) {
		print "aixterm 4bit(16) bright color\n";
		print " FG: SGR 90..97 m  BG: SGR 100..107 m\n";
		for ($f = 0; $f < 8; $f++) {
			for ($b = 0; $b < 8; $b++) {
				printf("\x1b[%d;%dm %3d/%3d ", $f + 90, $b + 100, $f + 90 , $b + 100);
			}
			print "\x1b[0m";
			print "\n";
		}
	}

	if (1) {
		print "PC-Style 4bit(16) bright color\n";
		print " FG: SGR 30..37 m  BG: SGR 40..47 m\n";
		for ($f = 0; $f < 8; $f++) {
			for ($b = 0; $b < 8; $b++) {
				print "\x1b[1m";
				printf("\x1b[%d;%dm %3d/%3d ", $f + 30, $b + 40, $f + 30, $b + 40);
			}
			print "\x1b[0m";
			print "\n";
		}
		print "\x1b[0m";
		print "\n";
	}

	if (1) {
		print "PC 4bit(16) color\n";
		print " FG: SGR 38 ; 5 ; n m  BG: 48 ; 5 ; n m (256color only)\n";
		for ($f = 0; $f < 8; $f++) {
			for ($b = 0; $b < 8; $b++) {
				printf("\x1b[38;5;%dm\x1b[48;5;%dm %3d/%3d ", $f, $b, $f, $b);
			}
			print "\x1b[0m";
			print "\n";
		}
		for ($f = 0; $f < 8; $f++) {
			for ($b = 0; $b < 8; $b++) {
				printf("\x1b[38;5;%dm\x1b[48;5;%dm %3d/%3d ", $f + 8, $b + 8, $f + 8, $b + 8);
			}
			print "\x1b[0m";
			print "\n";
		}
	}
}

# DECSCNM 反転表示モード
# DECSET ESC [ ? 5 h  設定
#  echo -en "\033[?5h"
# DECRST ESC [ ? 5 l  解除
#  echo -en "\033[?5l"
if (1) {
	print "==================== DECSCNM test\n";
	for ($i = 0; $i < 50; $i++) {
		if (($i % 2) == 0) {
			print "\x1b[?5l";
		} else {
			print "\x1b[?5h";
		}
		sleep(1);
	}
	print "\x1b[?5l";
}
