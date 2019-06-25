#!/usr/bin/perl
use utf8;

open(FILE, "NamesList.txt") || die "Cannot open width file.";
$ostart = 0;
$oend = 0;
while($a = <FILE>) {
	if ($a =~ /^([0-9A-F]+)\s+(.*)$/) {
		$start = hex $1;
		$name = $2;
		if ($name =~ /COMBINING/ || $name =~ /EMOJI MODIFIER/) {
			if ($ostart == 0) {
				$ostart = $start;
				$oend = $start;
				$comment = $name;
			} else {
				if ($oend + 1 == $start) {
					$oend = $start;
				}
			}
		} else {
			if ($ostart != 0) {
				printf("{ 0x%06x, 0x%06x },		// '%s'\n", $ostart, $oend, $comment);
				$ostart = 0;
			}
		}
	}
}
if ($ostart != 0) {
	printf("{ 0x%06x, 0x%06x },\n", $ostart, $oend);
}

