#!/usr/bin/perl
# 日本語環境用に teraterm.ini を設定する

use strict;
use warnings;
use utf8;

my $in_file = $ARGV[0];

my $vtfont = 'ＭＳ ゴシック,0,-16,128';
#my $vtfont = 'Terminal,0,-19,128';

binmode STDOUT, ":crlf:encoding(cp932)";
open(INI, '<:crlf:encoding(cp932)', $in_file);
while(<INI>){
	s/^(Language\s*=).*$/$1Japanese/;
	s/^(VTFont\s*=).*$/$1$vtfont/;
	s/^(TEKFont\s*=).*$/$1Terminal,0,8,128/;
	s/^(TCPPort\s*=).*$/${1}22/;
	print;
}
close INI;
