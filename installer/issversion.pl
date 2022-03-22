#!/usr/bin/perl

use strict;
use warnings;
use utf8;

open(ISS, '<:crlf:encoding(UTF-8)', 'teraterm.iss');
while(<ISS>){
	if (/^#define AppVer "(.+)"$/) {
		print $1;
		last;
	}
}
close ISS;
