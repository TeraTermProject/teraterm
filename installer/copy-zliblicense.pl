#!/usr/bin/perl -w

require 5.8.0;
use strict;
use Encode;
use utf8;
use Getopt::Long qw(:config posix_default no_ignore_case gnu_compat);

my($in, $out, $coding, $lf, $result);

# default setting
$lf = "crlf";

$result = GetOptions('in|i=s'     => \$in,
                     'out|o=s'    => \$out,
                     'lf|l=s'     => \$lf);

if (!(defined($in) && defined($out))) {
	die "Usage: $0 --in file --out file [ --lf line_format ]\n";
}

open (IN,  "<:$lf",   $in);
while (<IN>) {
	last if $_ =~ /Copyright notice:/;
}

open (OUT, '>:crlf', $out);
print OUT $_;

while (<IN>) {
	print OUT $_;
}
close OUT;
close IN;

my(@filestat) = stat $in;
utime $filestat[8], $filestat[9], $out;
