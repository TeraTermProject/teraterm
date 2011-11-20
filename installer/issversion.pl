#!/usr/bin/perl

open ISS, 'teraterm.iss';
while(<ISS>){
	if (/^#define AppVer "(.+)"$/) {
		print $1;
		last;
	}
}
close ISS;
