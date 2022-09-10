#!/usr/bin/perl

use utf8;
use strict;
use warnings;
use Class::Struct;

local $| = 1;

sub Pause {
	printf("pause(hit enter)\n");
	<STDIN>;
}

sub ShowAttributesSGR {
	my @AttrTestsSGR = (
		{
			Enter => "\x1b[0m",
			Str => "Normal            / SGR 0",
		},
		{
			Enter => "\x1b[39m\x1b[49m",
			Str => "Normal            / SGR 39, 49",
		},
		{
			Enter => "\x1b[1m",
			Str => "Bold              / SGR 1",
		},
		{
			Enter => "\x1b[4m",
			Str => "Underline         / SGR 4",
		},
		{
			Enter => "\x1b[5m",
			Str => "Blink(Slow blink) / SGR 5",
		},
		{
			Enter => "\x1b[7m",
			Str => "Reverse           / SGR 7",
		},
		{
			Enter => "\x1b[0m",
			Str => "https://ttssh2.osdn.jp/  SGR 0(Normal) + URL string\n",
		},
		{
			Enter => "\x1b[0m\x1b[31m",
			Str => "FG Red            / SGR 0 + SGR 31(FG Red)",
		},
		{
			Enter => "\x1b[0m\x1b[42m",
			Str => "BG Red            / SGR 0 + SGR 42(BG Green)",
		},
		{
			Enter => "\x1b[31m\x1b[42m",
			Str => "FG Red + BG Green / SGR 31(FG Red) + SGR 42(BG Green)",
		},
		{
			Enter => "\x1b[7m\x1b[31m\x1b[42m",
			Str => "Reverse           / SGR 7 + SGR 31(FG Red) + SGR 42(BG Green)",
		},
		{
			Enter => "\x1b[1m\x1b[41m",
			Str => "BOLD + BG RED     / SGR 1 + SGR 41(BG Red)",
		},
		{
			Enter => "\x1b[5m\x1b[46m",
			Str => "Blink(Slow blink) / SGR 5 + SGR 46(BG Cyan)",
		},
		{
			Enter => "\x1b[5m\x1b[106m",
			Str => "Blink(Slow blink) / SGR 5 + SGR 106(BG Bright Cyan)",
		},
		{
			Enter => "\x1b[4m\x1b[44m",
			Str => "Underline         / SGR 4 + SGR 44(BG Blue)",
		},
		{
			Enter => "\x1b[4m\x1b[104m",
			Str => "Underline         / SGR 4 + SGR 104(BG Bright Blue)",
		},
		{
			Enter => "\x1b[30m\x1b[40m",
			Str => "Black+Black       / SGR 30 + SGR 40 (FG&BG Black)",
		},
		{
			Enter => "\x1b[37m\x1b[47m",
			Str => "White+White(8)    / SGR 37 + SGR 47 (FG&BG White(8color)/Gray(16,256color))",
		},
		{
			Enter => "\x1b[97m\x1b[107m",
			Str => "White+White       / SGR 97 + SGR 107 (FG&BG White(16,256color))",
		}
		);

	print "\x1b[0m";
	print "==================== support attributes\n";
	for my $i (0 .. $#AttrTestsSGR) {
		print "$AttrTestsSGR[$i]{Enter}";
		printf("%02d ", $i);
		print "$AttrTestsSGR[$i]{Str}";
		print "\x1b[0m\n";
	}
}

sub ShowAttributesREVERSE {
	my @AttrTestsReverse = (
		{
			Enter => "\x1b[0m",
			Str => "Normal            / SGR 0",
		},
		{
			Enter => "\x1b[7m",
			Str => "Reverse           /    + SGR 7",
		},
		{
			Enter => "\x1b[32m",
			Str => "FG ANSIGreen      / SGR 32(FG ANSIGreen)",
		},
		{
			Enter => "\x1b[32m\x1b[7m",
			Str => "FG ANSIGreen + R  /    + SGR 7",
		},
		{
			Enter => "\x1b[41m",
			Str => "BG ANSIRed        / SGR 41(BG ANSIRed)",
		},
		{
			Enter => "\x1b[41m\x1b[7m",
			Str => "BG ANSIRed + R    /     + SGR 7",
		},
		{
			Enter => "\x1b[32m\x1b[41m",
			Str => "FG ANSIGreed + BG ANSIRed     / SGR 32 + SGR 41(FG ANSIGreen + FG ANSIRed)",
		},
		{
			Enter => "\x1b[32m\x1b[41m\x1b[7m",
			Str => "FG ANSIGreed + BG ANSIRed + R /    + SGR 7",
		},
		);
	print "\x1b[0m";
	print "==================== reverse test\n";
	for my $i (0 .. $#AttrTestsReverse) {
		print "$AttrTestsReverse[$i]{Enter}";
		printf("%02d ", $i);
		print "$AttrTestsReverse[$i]{Str}";
		print "\x1b[0m\n";
	}
}

sub ShowANSIColor {
	print "==================== ANSI color\n";

	print "\x1b[0m";

	print "3bit(8) Standard color / 4bit(16) Darker color\n";
	print " FG: SGR 30..37 m  BG: SGR 40..47 m\n";
	for (my $f = 0; $f < 8; $f++) {
		for (my $b = 0; $b < 8; $b++) {
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
		for (my $f = 0; $f < 8; $f++) {
			for (my $b = 0; $b < 8; $b++) {
				printf("\x1b[%d;%dm %3d/%3d ", $f + 90, $b + 100, $f + 90 , $b + 100);
			}
			print "\x1b[0m";
			print "\n";
		}
	}

	if (1) {
		print "PC-Style 4bit(16) bright color\n";
		print " FG: SGR 30..37 m  BG: SGR 40..47 m\n";
		for (my $f = 0; $f < 8; $f++) {
			for (my $b = 0; $b < 8; $b++) {
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
		for (my $f = 0; $f < 8; $f++) {
			for ($b = 0; $b < 8; $b++) {
				printf("\x1b[38;5;%dm\x1b[48;5;%dm %3d/%3d ", $f, $b, $f, $b);
			}
			print "\x1b[0m";
			print "\n";
		}
		for (my $f = 0; $f < 8; $f++) {
			for (my $b = 0; $b < 8; $b++) {
				printf("\x1b[38;5;%dm\x1b[48;5;%dm %3d/%3d ", $f + 8, $b + 8, $f + 8, $b + 8);
			}
			print "\x1b[0m";
			print "\n";
		}
	}
}

# OSCシーケンス 色設定
sub TestOSC()
{
	print "==================== OSC test\n";
	Pause();

	printf("black-white\n");
	for (my $i = 0; $i < 16; $i++) {
		printf("\x1b]4;%d;#%1x%1x%1x\x1b\\", $i, $i, $i, $i);
	}
	Pause();

	printf("reset color\n");
	for (my $i = 0; $i < 16; $i++) {
		printf("\x1b]104;%d\x1b\\", $i);
	}
}

# DECSCNM 反転表示モード
# DECSET ESC [ ? 5 h  設定
#  echo -en "\033[?5h"
# DECRST ESC [ ? 5 l  解除
#  echo -en "\033[?5l"
sub ReverseVideoMode {
	print "==================== DECSCNM test\n";
	print "flushing..\n";
	for (my $i = 0; $i < 50; $i++) {
		if (($i % 2) == 0) {
			print "\x1b[?5l";
		} else {
			print "\x1b[?5h";
		}
		sleep(1);
	}
	print "\x1b[?5l";
}

my $arg = (@ARGV != 0) ? $ARGV[0] : "";

print "\x1b[0m";
print "SGR(Select Graphic Rendition) test\n";

 FINISH: for(;;)
{
	my $s;
	if (length($arg) != 0) {
		print "> $arg\n";
		$s = $arg;
		$arg = "";
	} else {
		print <<EOF;
a   attribute test patterns
c   ansi color list
o   Test OSC
r   reverse test patterns
R   reverse video
q   quit
EOF
		print "> ";
		$s = <STDIN>;
	}
	$s =~ s/[\n\r]//g;

	for(;;) {
		if (length($s) == 0) {
			next FINISH;
		}
		my $c = substr($s, 0, 1);
		$s = substr($s, 1);

		if ($c eq 'q') {
			last FINISH;
		} elsif ($c eq 'a') {
			ShowAttributesSGR();
		} elsif ($c eq 'c') {
			ShowANSIColor();
		} elsif ($c eq 'o') {
			TestOSC();
		} elsif ($c eq 'R') {
			ReverseVideoMode();
		} elsif ($c eq 'r') {
			ShowAttributesREVERSE();
		} else {
			print " ?\n"
		}
	}
}
print "\x1b[0m";
print "finish\n";
