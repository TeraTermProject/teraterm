
#
# Shift_JIS‚ğƒL[‚Æ‚µ‚Ä¸‡‚Éo—Í‚·‚é
#

$file = 'SHIFTJIS_TXT.htm';

&read_mapfile($file);
exit();

sub read_mapfile {
	my($file) = @_;
	my(%table, $val, $key);

	open(FP, $file) || die "error"; 
	while ($line = <FP>) {
		if ($line =~ /^\#/) {next;}
		if ($line =~ m+^\/+) {next;}
		if ($line =~ m+^\<+) {next;}
		$line =~ s/^\s+//;
		@column = split(/\s+/, $line);

		$val = int(hex($column[0])); # Unicode
#		print "$column[0] -> $column[1] ($val)\n";
		$table{$val} = hex($column[1]);
#		printf "%d => %x\n", $val, $table{$val};
	}
	close(FP);

	foreach $key (sort {$a <=> $b} keys %table) {
		printf "	{ 0x%04X, 0x%04X },\n", $key, $table{$key};
	}
}

