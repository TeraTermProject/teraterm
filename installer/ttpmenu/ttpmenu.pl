
#
# Export Tera Term Menu registry to ini file.
#   with ActivePerl
#
# Usage:
# c:\>perl ttpmenu.pl > ttpmenu.ini
#

use Win32::Registry;
use strict;

my $TTMREG = "Software\\ShinpeiTools\\TTermMenu";
my $tips;
my $key;
my @subkeys;

PrintSectionName("TTermMenu");
ExportIniFile($TTMREG);

$HKEY_CURRENT_USER->Open($TTMREG, $tips) or die "Can not open registry";
$tips->GetKeys(\@subkeys);
$tips->Close();
foreach $key (@subkeys) {
#	print "$key\n";
	PrintSectionName("$key");
	ExportIniFile($TTMREG . "\\" . $key);
}


exit(0);


sub PrintSectionName {
	my($name) = @_;
	
	print "[$name]\n";
}


sub ExportIniFile {
	my($path) = @_;
	my($tips);
	my(%vals);
	my($key, $RegType, $RegValue, $RegKey, @bytes);

	$HKEY_CURRENT_USER->Open($path, $tips) or die "Can not open registry";

	$tips->GetValues(\%vals);
	foreach $key (keys(%vals)) {
		$RegType     = $vals{$key}->[1];
		$RegValue     = $vals{$key}->[2];
		$RegKey     = $vals{$key}->[0];
		print "$RegKey=";
		if ($RegType == REG_DWORD) {
			printf "%08x\n", $RegValue;
			
		} elsif ($RegType == REG_BINARY) {
			@bytes = unpack('C*', $RegValue);
			printf "%02x ", $_ foreach @bytes;
			print "\n";
		
		} else {
			print "$RegValue\n";
		}
	}
	print "\n";

	$tips->Close();
}

