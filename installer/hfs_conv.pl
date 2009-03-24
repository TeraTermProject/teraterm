#
# Unicodeをキーとして昇順にソートする
#
# >perl hfs_conv.pl > hfs_plus.map

$file = 'UNICODE DECOMPOSITION TABLE.htm';

print <<EOD;
typedef struct hfsplus_codemap {
	unsigned short illegal_code;
	unsigned short first_code;
	unsigned short second_code;
} hfsplus_codemap_t;

/*
 * cf. http://developer.apple.com/technotes/tn/tn1150table.html 
 *
 */
static hfsplus_codemap_t mapHFSPlusUnicode[] = {
EOD

&read_mapfile($file);

print <<EOD;
};
EOD

exit();

sub read_mapfile {
	my($file) = @_;
	my(%table, $val, $key);

	$illegal = '';
	$first = '';
	$second = '';

	open(FP, $file) || die "error"; 
	while ($line = <FP>) {
#		print "$line\n";
		if ($line =~ m|<P>0x(....)</P>|) {
			$illegal = hex($1);
		}
		if ($line =~ m|<P>0x(....) 0x(....)</P>|) {
			$first = hex($1);
			$second = hex($2);

			push(@key, "$illegal,$first,$second");

			#		printf "	 {0x%04X,   0x%04X, 0x%04X},\n", $illegal, $first, $second;
		}
	}
	close(FP);

	# $firstをキーとしてソートする
	$k = @key - 1;
	while ($k >= 0) {
		$j = -1;
		for ($i = 1; $i <= $k; $i++) {
			@m = split(/,/, $key[$i-1]);
			@n = split(/,/, $key[$i]);
			if ($m[1] > $n[1]) {
				$j = $i - 1;
				$t = $key[$j];
				$key[$j] = $key[$i];
				$key[$i] = $t;
			}
		}
		$k = $j;
	}
	
	foreach $s (@key) {
		@n = split(/,/, $s);
		printf "	 {0x%04X,   0x%04X, 0x%04X},\n", $n[0], $n[1], $n[2];
	}

#	foreach $key (sort {$a <=> $b} keys %table) {
#		@n = split(/,/, $table{$key});
#		printf "	 {0x%04X,   0x%04X, 0x%04X},\n", $n[0], $key, $n[1];
#	}

}

