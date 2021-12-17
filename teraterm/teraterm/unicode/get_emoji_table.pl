#!/usr/bin/perl
use utf8;
use strict;
use warnings;

binmode STDOUT, ":utf8";

sub output {
	my ($fh, $start, $end, $comment) = @_;

	if ($start >= 0x80) {
		printf($fh "{ 0x%06x, 0x%06x }, // %s\n", $start, $end, $comment);
	}
}

my $fname_out = "unicode_emoji.tbl";
if (@ARGV == 1) {
	$fname_out = $ARGV[0];
}

my $src_file = "emoji-data.txt";
open(my $fh, "<:utf8", $src_file) || die "Cannot open $src_file.";
open(my $fh_out, ">:crlf:utf8", $fname_out) || die "Cannot open $fname_out.";
printf($fh_out "\x{feff}");	# BOM
printf($fh_out "// $src_file\n");

my $block = 0;
my $start;
my $end;
my $comment;
my $ostart;
my $ocomment;
my $oend = 0;
while(my $a = <$fh>) {
	if ($a =~ /^# ======/) {
		$block = $block + 1;
		if ($block == 2) {
			last;
		}
	}

	if ($a =~ /^([0-9A-F]+)\.\.([0-9A-F]+).+\)\s+(.+)$/) {
		$start = hex $1;
		$end = hex $2;
		$comment = $3;
	} elsif ($a =~ /^([0-9A-F]+).+\)\s+(.+)$/) {
		$start = hex $1;
		$end = $start;
		$comment = $2;
	} else {
		next;
	}

	if ($oend == 0) {
		$ostart = $start;
		$oend = $end;
		$ocomment = $comment;
	} elsif ($oend + 1 == $start) {
		# combine
		$oend = $end;
	} else {
		output($fh_out, $ostart, $oend, $ocomment);
		$ostart = $start;
		$oend = $end;
		$ocomment = $comment;
	}
}

if ($oend != 0) {
	output($fh_out, $ostart, $oend, $ocomment);
}
