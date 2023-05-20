#! /usr/bin/perl
#
# show code chart
#  0x00 - 0xff
#
use utf8;
use strict;
use warnings;

use Getopt::Long 'GetOptions';

my $code = "";
my $usage = 0;

local $| = 1;

GetOptions(
	'code=s' => \$code,
	'help' => \$usage,
);

sub IsC0 {
	my $ch= shift;
	if (0x00 <= $ch && $ch <= 0x1f) {
		return 1;
	}
	return 0;
}

sub IsC1 {
	my $ch= shift;
	if (0x80 <= $ch && $ch <= 0x9f) {
		return 1;
	}
	return 0;
}

sub IsPrintableUTF8 {
	my $ch= shift;
	if (IsC0($ch) || IsC1($ch)) {
		return 0;
	}
	if ($ch == 0x7f) {
		# DEL
		return 0;
	}
	return 1;
}

sub IsPrintableShiftJIS {
	my $ch= shift;
	if (IsC0($ch) || IsC1($ch)) {
		return 0;
	}
	if ($ch == 0x7f) {
		# DEL
		return 0;
	}
	if ($ch == 0x80 || $ch == 0xa0) {
		return 0;
	}
	if (0xe0 <= $ch && $ch <= 0xff) {
		return 0;
	}
	return 1;
}

sub IsPrintableJIS {
	my $ch= shift;
	if (IsC0($ch) || IsC1($ch)) {
		return 0;
	}
	if ($ch == 0x7f) {
		# DEL
		return 0;
	}
	if (0x80 <= $ch && $ch <= 0xa0) {
		return 0;
	}
	if (0xe0 <= $ch && $ch <= 0xff) {
		return 0;
	}
	return 1;
}

sub IsPrintableEUCJP {
	my $ch= shift;
	if (IsC0($ch)) {
		return 0;
	}
	if ($ch == 0x7f) {
		# DEL
		return 0;
	}
	if ($ch >= 0x80) {
		return 0;
	}
	return 1;
}

my %check =
(
 'utf8' => \&IsPrintableUTF8,
 'utf8-bin' => \&IsPrintableUTF8,
 'iso8859' => \&IsPrintableUTF8,
 'iso646' => \&IsPrintableEUCJP,
 'sjis' => \&IsPrintableShiftJIS,
 'shift_jis' => \&IsPrintableShiftJIS,
 'euc-jp' => \&IsPrintableEUCJP,
 'jis' => \&IsPrintableJIS,
 );

sub usage {
	print << 'EOS';
option:
  --code=[code]
EOS
	for my $key (sort keys %check) {
		print "      $key\n";
	}
}

my @C0_str = (
"NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
"BS ", "HT ", "LF ", "VT ", "FF ", "CR ", "SO ", "SI ",
"DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
"CAN", "EM ", "SUB", "ESC", "FS ", "GS ", "RS ", "US " );

my @C1_str = (
"PAD", "HOP", "BPH", "NBH", "IND", "NEL", "SSA", "ESA",
"HTS", "HTJ", "VTS", "PLD", "PLU", "RI ", "SS2", "SS3",
"DCS", "PU1", "PU2", "STS", "CCH", "MW ", "SPA", "EPA",
"SOS", "SGC", "SCI", "CSI", "ST ", "OSC", "PM ", "APC" );

sub ShowHeader
{
	print "      |";
	for (my $i = 0; $i < 16; $i++) {
		printf(" +%x|", $i);
	}
	print "\n";
	printf("------+");
	for (my $y = 0; $y < 15; $y++) {
		printf("---+");
	}
	print "---|\n";
}

my $invoke_enter = "";
my $invoke_leave = "";
my $cell = 1;

sub ShowCode
{
	my $code = shift;
	my $s = shift;
	my $e = shift;

	ShowHeader();
	for (my $y = int($s/16); $y < int(($e+1)/16); $y++) {
		printf(" 0x%01xX |", $y);
		for (my $x = 0; $x < 16; $x++) {
			my $c = $y * 0x10 + $x;
			if ($c == 0x20) {
				printf("SP |");
			} elsif ($c == 0x7f) {
				printf("DEL|");
			} elsif ($check{$code}->($c)) {
				my $ch = chr($c);
				if ($cell == 1) {
					print " " . $invoke_enter . $ch . $invoke_leave . " |";
				} else {
					print $invoke_enter . $ch . $invoke_leave . " |";
				}
			} else {
				if ($c >= 0 && $c <= 0x1f) {
					printf("%s|", $C0_str[$c]);
				} elsif ($c >= 0x80 && $c <= 0x9f) {
					printf("%s|", $C1_str[$c-0x80]);
				} else {
					printf("---|");
				}
			}
			#printf(" %02x|", $c);
			#if (ord($ch) == 0xa0) {
			#	exit(1);
			#}
		}
		print "\n";
	}
}

if ($usage) {
	usage();
	exit(0);
}

if (length($code) != 0) {
	my $find = 0;
	for my $key (sort keys %check) {
		if (index($key, lc($code)) != -1) {
			$find = 1;
			$code = $key;
			last;
		}
	}
	if ($find == 0) {
		print "check code\n";
		usage();
		exit(0);
	}

	# output
	if ($code eq "utf8") {
		# utf8 encode
		binmode STDOUT, ":utf8";
	} else {
		binmode STDOUT;
	}
	printf("code=$code\n");
	ShowCode($code);
}

# interactive
 FINISH: for(;;)
{
	my $s;
	binmode STDOUT;
	print <<EOF;
a   All
      all chars, 0x00...0xFF
      ISO/IEC 8859(=Unicode)
      ISO/IEC 2022 8bit
u   Unicode + UTF-8
      CES(character encoding scheme) = UTF-8
      U+0000...U+00FF
7   ISO/IEC 2022 7bit (jis)
s   Shift_JIS
e   Japanese/EUC (euc-jp)
j   JIS
q   quit
EOF
	print "> ";
	$s = <STDIN>;
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
			ShowCode('utf8', 0x00, 0xff);
		} elsif ($c eq 'u') {
			binmode STDOUT, ":utf8";
			ShowCode('utf8', 0x00, 0xff);
			binmode STDOUT;
		} elsif ($c eq '8') {
			ShowCode('iso8859', 0x00, 0xff);
		} elsif ($c eq '7') {
			print "GL(0x20-0x7F) <- G0:\n";
			$invoke_enter = chr(0x0f);
			$invoke_leave = chr(0x0f);
			ShowCode('utf8', 0x00, 0x7f);
			print "GL(0x20-0x7F) <- G1:\n";
			$invoke_enter = chr(0x0e);
			$invoke_leave = chr(0x0f);
			ShowCode('utf8', 0x00, 0x7f);
			print "GL(0x20-0x7F) <- G3, 0x21XX:\n";
			$invoke_enter = chr(0x1b) . "o" . chr(0x21); # LS3(GL<-G3) + 0x21xx
			$invoke_leave = chr(0x0f); # GL<-G0
			$cell = 2;
			ShowCode('utf8', 0x00, 0x7f);
			$invoke_enter = "";
			$invoke_leave = "";
			$cell = 1;
			# GL <- G0
			print chr(0x0f);
		} elsif ($c eq 's') {
			ShowCode('shift_jis', 0x00, 0xff);
		} elsif ($c eq 'e') {
			ShowCode('euc-jp', 0x00, 0xff);
		} elsif ($c eq 'j') {
			print "C0(0x00-0x1F) & GL(0x20-0x7F):\n";
			ShowCode('jis', 0x00, 0x7f);
			print "C1(0x80-0x9F) & GR(0xA0-0xFF):\n";
			ShowCode('jis', 0x80, 0xff);
		} else {
			print " ?\n"
		}
	}
}
