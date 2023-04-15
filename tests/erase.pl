#!/usr/bin/perl

# 消去系のエスケープシーケンスのテスト
# テストの動作のポイント
#   - マウスで選択した領域が描画後も残ったままになる
#   - 2cell文字が半分残った状態にならない

use utf8;
use strict;
use warnings;

local $| = 1;
binmode STDOUT, ':encoding(UTF-8)';

sub Wait() {
	print("[[hit return key]]");
	my $s = <STDIN>;
}

sub Fill {
	my $ys = shift;
	my $ye = shift;

	#print "\x1b[2J";
	print "\x1b[1;1H";
	my $wide_count = 0;
	for (my $y = $ys; $y < $ye; $y++) {
		if (($y % 3) == 0) {
			for (my $x = 0; $x < 80; $x++) {
				printf("%d", $x %10);
			}
		} else {
			my $w = 40 - ($wide_count % 2);
			printf("-") if $w != 40;
			for (my $x = 0; $x < $w; $x++) {
				printf("%s", substr("□■", ($x) % 2, 1));
			}
			printf("-") if $w != 40;
			$wide_count++;
		}
	}
	print "\x1b[10;1H";
}

sub EL {
	my $ps = shift;
	Fill(0, 10);
	print "\x1b[1;40H\x1b[${ps}K";
	print "\x1b[2;40H\x1b[${ps}K";
	print "\x1b[3;40H\x1b[${ps}K";
	print "\x1b[4;39H ^cursor pos ";
	print "\n";
	Wait();
}

sub ECH {
	Fill(0, 10);
	print "\x1b[1;40H\x1b[10X";
	print "\x1b[2;40H\x1b[10X";
	print "\x1b[3;40H\x1b[10X";
	print "\x1b[4;40H\x1b[9X";
	print "\x1b[5;40H\x1b[9X";
	print "\x1b[6;40H\x1b[9X";
	print "\x1b[7;39H ^cursor pos ";
	print "\n";
	Wait();
}

sub ED {
	my $ps = shift;
	for (my $y = 7; $y < 14; $y++) {
		Fill(0, 20);
		print "\x1b[1;1H\n";
		print "\x1b[${y};40H\x1b[${ps}J";
		print "\x1b[1;1H\n";
		Wait();
	}
}

my $arg = (@ARGV != 0) ? $ARGV[0] : "";

print "\x1b[0m";
print "ERASE test\n";

 FINISH:

	for(;;)
{
	my $s;
	if (length($arg) != 0) {
		print "> $arg\n";
		$s = $arg;
		$arg = "";
	} else {
		print <<EOF;
1   EL 0 
2   EL 1 
3   EL 2 
4   ECH 
5   ED 0 
6   ED 1 
q   quit 
EOF
		print "> ";
		$s = <STDIN>;
	}
	$s =~ s/[\n\r]//g;

	for(;;) {
		if (length($s) == 0) {
			last;
			#next FINISH;
		}
		my $c = substr($s, 0, 1);
		$s = substr($s, 1);

		if ($c eq 'q') {
			last FINISH;
		} elsif ($c eq '1') {
			EL(0);
		} elsif ($c eq '2') {
			EL(1);
		} elsif ($c eq '3') {
			EL(2);
		} elsif ($c eq '4') {
			ECH(2);
		} elsif ($c eq '5') {
			ED(0);
		} elsif ($c eq '6') {
			ED(1);
		} else {
			print "'$c' ?\n"
		}
	}
}
print "\x1b[2J";
print "\x1b[1;1H";
print "\x1b[0m";
print "finish\n";
