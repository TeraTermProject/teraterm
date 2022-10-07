# 結合文字 Spacing mark test

use utf8;

binmode STDOUT, ":utf8";
local $| = 1;

#
print "Malayalam\n";
print "\N{U+0d2e}\N{U+0d32}\N{U+0d2f}\N{U+0d3e}\N{U+0d33}\N{U+0d02}\n";

print "\N{U+0d2e}| U+0d2e 1cell\n";
print "\N{U+0d32}| U+0d32 1cell\n";
print "\N{U+0d2f}\N{U+0d3e}| U+0d2f U+0d3e(Spacing Mark) 2cell\n";
print "\N{U+0d33}\N{U+0d02}| U+0d33 U+0d02(Spacing Mark) 2cell\n";

print "\N{U+0d2f}| U+0d2f 1cell\n";

# repeat spacing mark
print "repeat spacing mark\n";
for ($i = 0 ; $i < 10; $i++) {
	printf("%d", $i%10);
}
print "\n";
print "\N{U+0d33}";
for ($i = 0 ; $i < 10; $i++) {
	print "\N{U+0d02}";
}
print "\n";
