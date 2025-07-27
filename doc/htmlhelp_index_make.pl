#! /usr/bin/perl

#
# HTMLヘルプのインデックスファイルを生成する
#
# Usage:
#  perl htmlhelp_index_make.pl ja html -o ja\Index.hhk
#

require 5.8.0;
use strict;
use warnings;
use utf8;
use Cwd;
use Getopt::Long;
use Encode;
use Encode::Guess;

binmode STDOUT, ":utf8";

my $out = "index.hhk";
my $verbose = 0;
my $result = GetOptions(
	'out|o=s' => \$out,
	'verbose' => \$verbose
	);

my $OUT;
open ($OUT, '>:crlf:encoding(shiftjis)', $out);

my @dirstack = ();

do_main($ARGV[0], $ARGV[1]);

close $OUT;
exit(0);

sub detect_encoding {
	my $filename = shift;
	open(my $fp, '<:raw', $filename) or return undef;
	local $/;
	my $data = <$fp>;
	close($fp);

	my $decoder = Encode::Guess->guess($data, 'shiftjis', 'utf8');
	if (ref($decoder)) {
		return $decoder->name;	# 例: 'shiftjis', 'utf8', 'euc-jp'
	} else {
		return undef;  # 判定できなかった場合
	}
}

# 文字コードを自動判定して、ファイルをオープンする
sub open_auto {
	my $filename = $_[0];
	my $enc = detect_encoding($filename);
	if ($verbose != 0) {
		print "read '$filename' $enc\n";
	}
	my $FP;
	if (($enc eq 'shiftjis') || ($enc eq 'ascii')) {
		open($FP, "<:crlf:encoding(sjis)", "$filename") || return;
	} elsif ($enc eq 'utf8') {
		open($FP, "<:crlf:encoding(utf8)", "$filename") || return;
	} else {
		die "bad encode '$filename' $enc\n";
	}
	return $FP;
}

sub do_main {
	my($path, $body) = @_;

	print $OUT <<'EOD';
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<HTML><HEAD>
<meta name="GENERATOR" content="TeraTerm Project">
<!-- Sitemap 1.0 -->
</HEAD><BODY>
<UL>
EOD

	push @dirstack, getcwd;
	chdir $path or die "cannot chdir to $path $!";
	get_file_paths($body);
	chdir pop @dirstack;

	print $OUT <<'EOD';
</UL>
</BODY></HTML>
EOD

}


sub get_file_paths {
	my ($top_dir)= @_;
	my @temp = ();

	#-- カレントの一覧を取得 --#
	opendir(DIR, $top_dir);
	@temp = readdir(DIR);
	closedir(DIR);
	foreach my $path (sort @temp) {
		next if( $path =~ /^\.{1,2}$/ );                # '.' と '..' はスキップ
		next if( $path =~ /^\.svn$/ );                  # '.svn' はスキップ

		my $full_path = "$top_dir" . '/' . "$path";

		if ($verbose != 0) {
			print "$full_path\r\n";
		}
		if( -d "$top_dir/$path" ){                      #-- ディレクトリの場合は自分自身を呼び出す
			&get_file_paths("$full_path");

		} else {
			check_html_file($full_path);

		}
	}
}

sub get_title {
	my $filename = $_[0];
	my $title = "";
	my($line, $no, $val);

	my $FP = open_auto($filename);
	$no = 1;
	while(1) {
		$line = <$FP>;
		if (!defined($line)) {
			last;  # EOFチェック
		};
#		$line = chomp($line);
#		print "$line\n";
		if ($line =~ /<TITLE>(.+)<\/TITLE>/i) {
#			print "$filename:$no: $1\n";
#			print "$line\n";
			$val = $1;
			$val =~ s/"/&#34;/g;  # 二重引用符をエスケープする
			$title = $val;
			last;
		}

		$no++;
	}
	close($FP);

	return $title;
}

sub check_html_file {
	my($filename) = shift;

	if ($filename !~ /.html$/) {
		return;
	}
	if ($filename =~ /#/) {
		die "bad filename '$filename'";
	}

	my $title = &get_title($filename);
	if ($title eq "") {
		# タイトルがないhtmlのときファイル名をタイトルとする
		$filename =~ /\/([^\/]+)\.html$/;
		$title = $1;
	}

	write_add_index($filename, $title);
}

sub write_add_index {
	my($filename, $title) = @_;

	print $OUT <<"EOD";
<LI><OBJECT type="text/sitemap">
<param name="Name" value="$title">
<param name="Local" value="$filename">
</OBJECT>
EOD
	if ($verbose != 0) {
		print "$title\r\n";
	}

}
