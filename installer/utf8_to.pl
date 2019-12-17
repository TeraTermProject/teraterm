use utf8;

my $code = $ARGV[0];
my $in = $ARGV[1];
my $out = $ARGV[2];

open(INP, "<:raw:utf8", $in) or die("error :$! $in");
open(OUT, ">:raw:encoding($code)", $out) or die("error :$! $out");

my $line = 1;
while (<INP>) {
	if ($line == 1 && $code !~ /utf/) {
		# remove BOM
		s/^\x{FEFF}//;
	}
	print OUT;
	$line++;
}
