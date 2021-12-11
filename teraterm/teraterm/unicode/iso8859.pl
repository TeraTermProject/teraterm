#!/usr/bin/perl
use strict;
use utf8;

my $fname_in;
my $fname_out;
if (@ARGV == 2) {
	$fname_in = $ARGV[0];
	$fname_out = $ARGV[1];
} else {
	my $n = @ARGV;
	printf "argv $n\n";
	exit 1;
}

open(FILE_IN, $fname_in) || die "Cannot open $fname_in.";
open(FILE_OUT, ">:crlf:utf8", $fname_out) || die "Cannot open $fname_out.";
printf FILE_OUT "\x{feff}";# BOM
printf FILE_OUT "// $fname_in\n";
while(my $a = <FILE_IN>) {
	if ($a =~ /^0x([0-9A-F]+)\s+0x([0-9A-F]+)\s+#\s+(.+)$/) {
		my $code = hex $1;
		my $unicode = hex $2;
		my $name = $3;
		printf FILE_OUT "{ 0x%02x, 0x%04x }, // %s\n", $code, $unicode, $name;
	}
}
