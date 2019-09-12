#!/usr/bin/perl
use utf8;

open(FILE, "emoji-data.txt") || die "Cannot open width file.";

$block = 0;
while($a = <FILE>) {
	if ($a =~ /^# ======/) {
		$block = $block + 1;
		if ($block == 2) {
			exit(1);
		}
	}

	if ($a =~ /^([0-9A-F]+)\.\.([0-9A-F]+).+\)\s+(.+)$/) {
		$start = hex $1;
		$end = hex $2;
		$comment = $3;
		if ($start > 0x100) {
			printf("{ 0x%06x, 0x%06x }, // %s\n", $start, $end, $comment);
		}
	} elsif ($a =~ /^([0-9A-F]+).+\)\s+(.+)$/) {
		$start = hex $1;
		$comment = $2;
		if ($start > 0x100) {
			printf("{ 0x%06x, 0x%06x }, // %s\n", $start, $start, $comment);
		}
	}
}
