use utf8;
use strict;

use Getopt::Long 'GetOptions';

my $code = "utf8";
my $usage = 0;

GetOptions(
	'code=s' => \$code,
	'help' => \$usage,
);

sub IsCL {
	my $ch = $_[0];
	if (0x00 <= $ch && $ch <= 0x1f) {
		return 1;
	}
	return 0;
}

sub IsCR {
	my $ch = $_[0];
	if (0x80 <= $ch && $ch <= 0x9f) {
		return 1;
	}
	return 0;
}

sub IsPrintableUTF8 {
	my $ch = $_[0];
	if (IsCL($ch) || IsCR($ch)) {
		return 0;
	}
	if ($ch == 0x7f) {
		# DEL
		return 0;
	}
	return 1;
}

sub IsPrintableShiftJIS {
	my $ch = $_[0];
	if (IsCL($ch) || IsCR($ch)) {
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
	my $ch = $_[0];
	if (IsCL($ch) || IsCR($ch)) {
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
	my $ch = $_[0];
	if (IsCL($ch)) {
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

if ($usage) {
	usage();
	exit(0);
}

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
printf("code=$code\n");

# output
if ($code eq "utf8") {
	# utf8 encode
	binmode STDOUT, ":utf8";
} else {
	binmode STDOUT;
}
local $| = 1;

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
for (my $y = 0; $y < 16; $y++) {
	printf(" 0x%01xX |", $y);
	for (my $x = 0; $x < 16; $x++) {
		my $c = $y * 0x10 + $x;
		my $ch = "-";
		if ($check{$code}->($c)) {
			$ch = chr($c);
		}
		printf(" %s |", $ch);
		#printf(" %02x|", $c);
		if ($ch == 0xa0) {
			exit(1);
		}
	}
	print "\n";
}

