# DEC Special Graphic - Unicode

use utf8;

binmode STDOUT, ":utf8";
local $| = 1;

#
print "      0123456789ABCDEF\n";

print "0x5f: ";
print "               \N{U+00a0}";
print "|\n";

print "0x60: ";
print "\N{U+25c6}";
print "\N{U+2592}";
print "\N{U+2409}";
print "\N{U+240c}";
print "\N{U+240d}";
print "\N{U+240a}";
print "\N{U+00b0}";
print "\N{U+00b1}";
print "\N{U+2424}";
print "\N{U+240b}";
print "\N{U+2518}";
print "\N{U+2510}";
print "\N{U+250c}";
print "\N{U+2514}";
print "\N{U+253c}";
print "\N{U+23ba}";
print "|\n";

print "0x70: ";
print "\N{U+23bb}";
print "\N{U+2500}";
print "\N{U+23bc}";
print "\N{U+23bd}";
print "\N{U+251c}";
print "\N{U+2524}";
print "\N{U+2534}";
print "\N{U+252c}";
print "\N{U+2502}";
print "\N{U+2264}";
print "\N{U+2265}";
print "\N{U+03c0}";
print "\N{U+2260}";
print "\N{U+00a3}";
print "\N{U+00b7}";
print " ";
print "|\n"
