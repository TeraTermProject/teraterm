#!/usr/bin/perl

open ISS, 'teraterm.iss';
while(<ISS>){
	if (/^#define AppVer "([0-9\.]+)"$/) {
		print $1;
		last;
	}
}
close ISS;
