#
# lngファイルの date= を現在日時にまとめて変更する
#
use utf8;
use POSIX 'strftime';

my $datestr = strftime("%Y-%m-%d", localtime());
my $dateline = "date=$datestr by TeraTerm Project\n";

my @lngs = glob("*.lng");

foreach my $lng (@lngs) {
	my $tmp = "update_lng_date.$$";

	if ($lng =~ /^Default.lng/) {
		next;
	}

	print "$lng\n";

	open(INP, "<:crlf:utf8", $lng) or die("error :$! $in");
	open(OUT, ">:crlf:encoding(utf8)", $tmp) or die("error :$! $out");

	my $line = 1;
	while (<INP>) {
		my $l = $_;
		if ($line < 10 && $l =~ /^date/) {
			$_ = $dateline;
		}
		print OUT;
		$line++;
	}

	close(INP);
	close(OUT);

	unlink $lng;
	rename $tmp, $lng;
}
