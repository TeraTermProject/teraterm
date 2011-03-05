#!/usr/bin/perl -w

require 5.8.0;
use strict;
use Encode;
use utf8;
use Getopt::Long;

my($in, $out, $coding, $lf, $result);
$result = GetOptions('in=s'     => \$in,
                     'out=s'    => \$out,
                     'coding=s' => \$coding,
                     'lf=s'     => \$lf);

open (IN,  "<:$lf:encoding($coding)",   $in);
open (OUT, '>:crlf:encoding(shiftjis)', $out);
while (<IN>) {
	print OUT $_;
}
close OUT;
close IN;
