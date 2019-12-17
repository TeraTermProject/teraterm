#!/usr/bin/perl
use utf8;

open(FILE, "EastAsianWidth.txt") || die "Cannot open width file.";
$ostart = -1;
while($a = <FILE>) {
	if ($a =~ /^([0-9A-F]+)\.\.([0-9A-F]+);([A-Za-z]+)/) {
		$start = hex $1;
		$end = hex $2;
		$type = $3;
		if ($type eq "Na") {
			$type = "n";
		}
	}
	elsif ($a =~ /^([0-9A-F]+);([A-Za-z]+)/) {
		$start = hex $1;
		$end = hex $1;
		$type = $2;
		if ($type eq "Na") {
			$type = "n";
		}
	} else {
		next;
	}

#	printf("-{ 0x%06x, 0x%06x, $type },\n", $start, $end);

	if ($otype eq $type
		#&& $oend+1 == $start	# 未定義部を飛ばしてつなげる
		)
	{
		# 1つ前とつなげる
		$oend = $end;
	} elsif ($ostart == -1) {
		$ostart = $start;
		$oend = $end;
		$otype = $type;
	} else {
		if (($otype eq "W") || ($otype eq "F") || ($otype eq "A") ||
			($otype eq "N") || ($otype eq "n")) {
			printf("{ 0x%06x, 0x%06x, '$otype' },\n", $ostart, $oend);
		}
		$ostart = $start;
		$oend = $end;
		$otype = $type;
	}
}
printf("{ 0x%06x, 0x%06x, '$type' },\n", $ostart, $oend);
