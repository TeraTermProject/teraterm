#!/usr/bin/perl
use utf8;
use strict;
use warnings;

binmode STDOUT, ":utf8";


my $width_file = "EastAsianWidth.txt";
my @width_table;
open(my $fh_w, "<:utf8", $width_file) || die "Cannot open $width_file.";
while(my $a = <$fh_w>) {
	my $start;
	my $end;
	my $type;
	if ($a =~ /^([0-9A-F]+)\.\.([0-9A-F]+)\s*;\s*([A-Za-z]+)/) {
		$start = hex $1;
		$end = hex $2;
		$type = $3;
	}
	elsif ($a =~ /^([0-9A-F]+)\s*;\s*([A-Za-z]+)/) {
		$start = hex $1;
		$end = hex $1;
		$type = $2;
	} else {
		next;
	}
	my %w = (start => $start, end => $end, type => $type);
	push @width_table, \%w;
}


my $emoji_file = "emoji-data.txt";
my $version = '';
open(my $fh_e, "<:utf8", $emoji_file) || die "Cannot open $emoji_file.";
while(my $a = <$fh_e>) {
	if ($a =~ /^# Version: (\d+\.\d(?:\.\d)?)/) {
		$version = $1;
		last;
	}
}

my $fname_out = "Unicode".$version."-emoji.txt";
if (@ARGV == 1) {
	$fname_out = $ARGV[0];
}

open(my $fh_out, ">:crlf:utf8", $fname_out) || die "Cannot open $fname_out.";
printf($fh_out "\x{feff}");	# BOM

my $block_start = 6;
my $block_count = 0;
my $block = 0;
my $start;
my $end;
my $comment;
my $ostart;
my $ocomment;
my $oend = 0;
while(my $a = <$fh_e>) {
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
	} else {
		output_range($fh_out, $ostart, $oend, $ocomment);
		$ostart = $start;
		$oend = $end;
		$ocomment = $comment;
	}
}

if ($oend != 0) {
	output_range($fh_out, $ostart, $oend, $ocomment);
}


sub moji_width {
	my ($c) = @_;

	foreach my $range (@width_table) {
		if ($c >= $range->{'start'}  &&
		    $c <= $range->{'end'}) {
			return $range->{'type'};
		}
	}

	return '';
}

sub output {
	my ($fh, $c) = @_;
	my $type = moji_width($c);
	printf($fh "%06X\t%s\t%s\n", $c, chr($c), $type);
}

sub output_range {
	my ($fh, $start, $end, $comment) = @_;

	if ($start >= 0x80 &&
	    $comment !~ /<reserved-/) {
		my $c = $start;
		do {
			output($fh, $c, $comment);
			$c++;
		} until ($c > $end)
	}
}

