#!/usr/bin/perl -w

require 5.24.0;
use strict;
use warnings;
use Encode;
use utf8;
use Getopt::Long qw(:config posix_default no_ignore_case gnu_compat);

my $PERL = $^X;

sub MarkdownToHTML {
	my $buf = $_[0];

	my $cmd = "$PERL Markdown_1.0.1/Markdown.pl";
#	my $cmd = 'cat';

	my $out_file = "MD_TO_HTML_$$" . "_md";
	my $in_file = "MD_TO_HTML_$$" . "_html";

	my $OUT;
	open ($OUT, ">:unix:encoding(utf8)", $out_file) or die("error :$! $out_file");
	print $OUT $buf;
	close $OUT;

	my $sys = "$cmd < $out_file > $in_file";
	my $r = system($sys);
	if ($r != 0) {
		print "r=$r\n";
		exit($r);
		# Can't spawn "cmd.exe" ... -> check $PATH
	}

	my $IN;
	open ($IN, "<:crlf:encoding(utf8)", $in_file) or die("error :$! $in_file");
	$buf = join "", <$IN>;
	close $IN;

	unlink $in_file;
	unlink $out_file;

	$buf;
}

binmode STDOUT, ":utf8";

my($in, $out, $result);

# default setting
my $coding = "shiftjis";
my $lf = "crlf";
my $type = "text";
my $zlib_special;

$result = GetOptions('in|i=s'       => \$in,
                     'out|o=s'      => \$out,
                     'coding|c=s'   => \$coding,
                     'lf|l=s'       => \$lf,
                     'type|t=s'     => \$type,
                     'zlib_special' => \$zlib_special);

if (!(defined($in) && defined($out))) {
	die "Usage: $0 --in file --out file [ --coding input_encoding ] [ --lf line_format ] [ --type type ]\n";
}

if ($in =~/\.md/) {
	$type = "markdown";
	$coding = "utf8";
}

my $IN;
if ($in eq "-") {
	binmode STDIN, ":$lf:encoding($coding)";
	$IN = *STDIN;
} else {
	open ($IN,  "<:$lf:encoding($coding)", $in) or die("error :$! $in");
}
if ($zlib_special) {
	while (<$IN>) {
		last if $_ =~ /Copyright notice:/;
	}
}
my $buf = join "", <$IN>;
$buf =~ s/\x{FEFF}//g; # remove all bom
close $IN;

if ($type =~ /markdown/i ) {
	$buf = &MarkdownToHTML($buf);
}

my $OUT;
if ($out eq "-") {
	binmode STDOUT, ":crlf:encoding(shiftjis)";
	$OUT = *STDOUT;
} else {
	open ($OUT, '>:crlf:encoding(shiftjis)', $out);
}
print $OUT $buf;
close $OUT;

if ($in ne "-") {
	my(@filestat) = stat $in;
	utime $filestat[8], $filestat[9], $out;
}
