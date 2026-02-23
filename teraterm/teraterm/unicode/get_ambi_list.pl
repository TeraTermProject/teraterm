#!/usr/bin/perl
use utf8;
use strict;
use warnings;

binmode STDOUT, ":utf8";


my $emoji_file = "emoji-data.txt";
my @emoji_table;
my $version = '';
open(my $fh_e, "<:utf8", $emoji_file) || die "Cannot open $emoji_file.";

my $block_start = 6;
my $block_count = 0;
my $block = 0;
while(my $a = <$fh_e>) {
	if ($a =~ /^# Version: (\d+\.\d(?:\.\d)?)/) {
		$version = $1;
		next;
	}

	if ($a =~ /^# ======/) {
		if ($block == 0) {
			$block_count = $block_count + 1;
			if ($block_count == $block_start) {
				$block = 1;
			}
		} else {
			last;
		}
	} elsif ($block == 0) {
		next;
	}

	my $start;
	my $end;
	my $comment;
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

	my %e = (start => $start, end => $end, comment => $comment);
	push @emoji_table, \%e;
}

my $fname_out = "Unicode".$version."-ambi.txt";
if (@ARGV == 1) {
	$fname_out = $ARGV[0];
}
open(my $fh_out, ">:crlf:utf8", $fname_out) || die "Cannot open $fname_out.";
printf($fh_out "\x{feff}");	# BOM

my $width_file = "EastAsianWidth.txt";
open(my $fh_w, "<:utf8", $width_file) || die "Cannot open $width_file.";
while(my $a = <$fh_w>) {
	my $start;
	my $end;
	my $type;
	my $cat;
	my $cnt = 1;
	my $comment;
	if ($a =~ /^([0-9A-F]+)\.\.([0-9A-F]+)\s+;\s+(\w+)\s+#\s+([&A-Za-z]+)\s+(?:\[(\d+)\])?\s+(.+)$/) {
		$start = hex $1;
		$end = hex $2;
		$type = $3;
		$cat = $4;
		$cnt = $5;
		$comment = $6;
	}
	elsif ($a =~ /^([0-9A-F]+)\s+;\s+(\w+)\s+#\s+([&A-Za-z]+)\s+(.+)$/) {
		$start = hex $1;
		$end = hex $1;
		$type = $2;
		$cat = $3;
		$comment = $4;
	} else {
		next;
	}

	if ($type eq "A" &&
	    $comment !~ /<private-/ &&
	    $comment !~ /^VARIATION SELECTOR/) {
		output_range($fh_out, $start, $end, $comment);
	}
}

sub is_emoji {
	my ($c) = @_;

	foreach my $range (@emoji_table) {
		if ($c >= $range->{'start'}  &&
		    $c <= $range->{'end'}) {
			return 1;
		}
	}

	return 0;
}

sub output {
	my ($fh, $c, $comment) = @_;
	$comment ||= '';
	my $emoji = '';
	if (is_emoji($c)) {
		$emoji = 'emoji';
	}
	printf($fh "%06X\t%s\t%s\t%s\n", $c, chr($c), $comment, $emoji);
}

sub output_range {
	my ($fh, $start, $end, $comment) = @_;
	my $loop = 1;

	my $c = $start;
	do {
		if ($loop == 1) {
			output($fh, $c, $comment);
		}
		else {
			output($fh, $c);
		}
		$c++;
		$loop++;
	} until ($c > $end)
}

