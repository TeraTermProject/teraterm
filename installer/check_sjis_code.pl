#! /usr/bin/perl

#
# 英語版ドキュメントに日本語が含まれていないかを調べる。
#
# Usage(on Cygwin):
#  perl ./check_sjis_code.pl ../doc/en/*/*/*.html > result.txt
#

for ($i = 0 ; $i < @ARGV ; $i++) {
#	print "$ARGV[$i]\n";
	check_sjis_code($ARGV[$i]);
}
exit(0);


# cf. http://charset.7jp.net/sjis.html
# ShiftJIS 文字

sub check_sjis_code {
	my($filename) = shift;
	local(*FP);
	my($line, $no);
	
	open(FP, "<$filename") || return;
	$no = 1;
	while ($line = <FP>) {
#		$line = chomp($line);
#		print "$line\n";
		if ($line =~ /([\xA1-\xDF]|[\x81-\x9F\xE0-\xEF][\x40-\x7E\x80-\xFC])/) {
			print "$filename:$no: $1\n";
			print "$line\n";
		}
		$no++;
	}
	close(FP);
}

