#!/usr/bin/perl
use utf8;

sub output {
	my ($start, $end, $comment) = @_;

	if ($start >= 0x80) {
		printf("{ 0x%06x, 0x%06x }, // %s\n", $start, $end, $comment);
	}
}

open(FILE, "emoji-data.txt") || die "Cannot open width file.";

$block = 0;
$oend = 0;
while($a = <FILE>) {
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
		output($ostart, $oend, $ocomment);
		$ostart = $start;
		$oend = $end;
		$ocomment = $comment;
	}
}

if ($oend != 0) {
	output($ostart, $oend, $ocomment);
}
