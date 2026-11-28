#
# Export TeraTerm Menu registry to ini file.
#   with Strawberry Perl
#
# Usage:
# c:\>perl ttpmenu.pl > ttpmenu.ini
#

use strict;
use warnings;
use Win32::TieRegistry (':REG_', Delimiter => '/', ArrayValues => 1);

my $TTMREG = 'HKEY_CURRENT_USER/Software/ShinpeiTools/TTermMenu';
my $changed = 0;

PrintSectionName('TTermMenu');
ExportIniFile($TTMREG);

my $root = $Registry->{$TTMREG} or die "Cannot open registry key: $TTMREG";

foreach my $key (keys %$root) {
    next unless $key =~ m!/$!;
    PrintSectionName($key);
    ExportIniFile($TTMREG . '/' . $key);
}

if ($changed == 1) {
	print STDERR "; ']' has been removed. check the section name.\n";
}

exit(0);

sub PrintSectionName {
    my ($name) = @_;

    $name =~ s!/$!!;
	if ($name =~ /\]/) {
		my $org = $name;
		# ] を削除。] 削除によりセクション名が重複する場合がある。
		$name =~ s/\]//g;
		print "[$name] ; <- [$org]\n";
		$changed = 1;
	} else {
		print "[$name]\n";
	}
}

sub ExportIniFile {
    my ($path) = @_;

    my $regkey = $Registry->{$path} or die "Cannot open registry key: $path";

    foreach my $RegKey (keys %$regkey) {
        next if $RegKey =~ m!/$!;
        my ($RegValue, $RegType) = @{ $regkey->{$RegKey} };
        $RegKey =~ s/^\///;
        print "$RegKey=";
        if ($RegType == REG_DWORD) {
            printf "%d\n", unpack("l", pack("L", hex($RegValue)));
        } elsif ($RegType == REG_BINARY) {
            my @bytes = unpack('C*', $RegValue);
            printf "%02X ", $_ foreach @bytes;
            print "\n";
        } else {
            print "$RegValue\n";
        }
    }
    print "\n";
}
