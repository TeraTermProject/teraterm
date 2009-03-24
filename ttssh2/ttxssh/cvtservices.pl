#!perl -w

my(%map) = ();

while (<>) {
  chop;
  s/#.*//;
  tr[A-Z][a-z];
  my(@tokens) = split;

  if (defined($tokens[1]) && $tokens[1] =~ m!^([0-9]+)/tcp$!) {
    my($port) = $1;
	$map{$tokens[0]} = $port;
	foreach $alias (@tokens[2..$#tokens]) {
      $map{$alias} = $port;
	}
  }
}

foreach $k (sort(keys(%map))) {
  print "  { $map{$k}, \"$k\" },\n";
}
