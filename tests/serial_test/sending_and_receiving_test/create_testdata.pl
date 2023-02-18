use strict;
use warnings;

my $data_size = 32*1024;
open(my $FD, ">testdata.bin") or die "error";
for(my $i = 0; $i < 32*1024/10; $i++) {
	print $FD "0123456789";
}
close($FD);
