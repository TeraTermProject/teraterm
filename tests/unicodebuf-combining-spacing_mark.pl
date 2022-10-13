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
print "\n";

print "Hello (in Malayalam)\n";
print "\N{U+0d28}\N{U+0d2e}\N{U+0d38}\N{U+0d4d}\N{U+0d15}\N{U+0d3e}\N{U+0d30}\N{U+0d02}\n";

print "\N{U+0d28}| U+0d28 1cell\n";
print "\N{U+0d2e}| U+0d2e 1cell\n";
print "\N{U+0d38}\N{U+0d4d}\N{U+0d15}\N{U+0d3e}| U+0d38 U+0d4d(Nonspacing Mark) U+0d15 U+0d3e(Spacing Mark) 2cell??\n";
print "\N{U+0d30}\N{U+0d02}| U+0d30 U+0d02(Spacing Mark) 2cell\n";
print "\n";

print "\N{U+307B}\N{U+309A}| U+307B U+309A (ほ + ゜ = ぽ)\n";
print "\n";

# repeat spacing mark
print "repeat spacing mark\n";
for ($i = 0 ; $i < 10 + 2; $i++) {
	printf("%d", ($i+1)%10);
}
print "\n";
print "\N{U+0d33}";
for ($i = 0 ; $i < 10; $i++) {
	print "\N{U+0d02}";
}
print "| U+0d33 + U+0d02 * 10 11cell\n";
