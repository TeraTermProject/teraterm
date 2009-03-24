#!/usr/bin/perl

open INI, $ARGV[0];
while(<INI>){
	s/^(Language\s*=).*$/$1Japanese/;
	s/^(Locale\s*=).*$/$1japanese/;
	s/^(CodePage\s*=).*$/${1}932/;
	s/^(VTFont\s*=).*$/$1‚l‚r –¾’©,0,-16,128/;
	s/^(TEKFont\s*=).*$/$1Terminal,0,8,128/;
	print;
}
close INI;
