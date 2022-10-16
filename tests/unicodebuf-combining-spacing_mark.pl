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
print "\N{U+0d38}\N{U+0d4d}\N{U+0d15}\N{U+0d3e}| U+0d38 U+0d4d(Nonspacing Mark) U+0d15 U+0d3e(Spacing Mark) 3cell\n";
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
print "\n";

# virama
print "Virama test\n";

# Devanagari
#  wget https://raw.githubusercontent.com/emacs-mirror/emacs/master/etc/HELLO -O - --quiet | grep Devanagari
#                     name                      prop                width
print "\N{U+0938}"; # DEVANAGARI LETTER SA                          +1
print "\N{U+094d}"; # DEVANAGARI SIGN VIRAMA    Virama              +0
print "\N{U+0924}"; # DEVANAGARI LETTER TA                          +1
print "\N{U+0947}"; # DEVANAGARI VOWEL SIGN E   Nonspacing mark     +0
print "| 2cell\n";

# Gujarati
#  wget https://raw.githubusercontent.com/emacs-mirror/emacs/master/etc/HELLO -O - --quiet | grep Gujarati
#                     name                      prop                width
print "\N{U+0ab8}"; # GUJARATI LETTER SA                            +1
print "\N{U+0acd}"; # GUJARATI SIGN VIRAMA      Virama              +0
print "\N{U+0aa4}"; # GUJARATI LETTER TA                            +1
print "\N{U+0ac7}"; # GUJARATI VOWEL SIGN E     Nonspacing mark     +0
print "| 2cell\n";

# Tamil
#  wget https://raw.githubusercontent.com/emacs-mirror/emacs/master/etc/HELLO -O - --quiet | grep Tamil
#                     name                      prop                width
print "\N{U+0bb4}"; # TAMIL LETTER LLLA                             +1
print "\N{U+0bcd}"; # TAMIL SIGN VIRAMA         Virama              +0
print "\N{U+0029}"; # RIGHT PARENTHESIS                             +1
print "| 2cell\n";
