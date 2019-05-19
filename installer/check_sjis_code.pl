#! /usr/bin/perl

#
# 英語版ドキュメントに日本語が含まれていないかを調べる。
#
# Usage(ActivePerl):
#  perl check_sjis_code.pl > result.txt
#

use Encode::Guess qw/shift-jis 7bit-jis/;

#my @exclude_files = qw(sourcecode.html);
my @exclude_files = qw();

get_file_paths('../doc/en/html');
exit(0);

sub get_file_paths {
	my ($top_dir)= @_;
	my @paths=();
	my @temp = ();

	#-- カレントの一覧を取得 --#
	opendir(DIR, $top_dir);
	@temp = readdir(DIR);
	closedir(DIR);
	foreach my $path (sort @temp) {
		next if( $path =~ /^\.{1,2}$/ );                # '.' と '..' はスキップ
		next if( $path =~ /^\.svn$/ );                  # '.svn' はスキップ

		my $full_path = "$top_dir" . '/' . "$path";
#		print "$full_path\r\n";                         # 表示だけなら全てを表示してくれる-------
		push(@paths, $full_path);                       # データとして取り込んでも前の取り込みが初期化される
		if( -d "$top_dir/$path" ){                      #-- ディレクトリの場合は自分自身を呼び出す
			&get_file_paths("$full_path");
		} elsif (-B $full_path) {
			# バイナリファイルはスキップ
			next;

		} elsif (&check_exclude_file($path)) {
			print "$full_path skipped\n";
			next;

		} else {
			check_sjis_code($full_path);

		}
	}
	return \@paths;
}


# 調査対象外のファイルかを調べる
sub check_exclude_file {
	my($fn) = shift;
	my($s);

	foreach $s (@exclude_files) {
		if ($fn eq $s) {
			return 1;
		}
	}
	return 0;
}


# cf. http://charset.7jp.net/sjis.html
# ShiftJIS 文字

sub check_sjis_code {
	my($filename) = shift;
	local(*FP);
	my($line, $no);
	
	open(FP, "<$filename") || return;
	$no = 1;
	while ($line = <FP>) {
#		$line = chomp($line);
#		print "$line\n";
		
		 my $enc = guess_encoding( $line, qw/ euc-jp shiftjis 7bit-jis utf8 / );

		if (ref $enc) {
#			printf "%s\n", $enc->name;
			if ($enc->name !~ /ascii/) {
#				printf "%s\n", $enc->name;
				print "$filename:$no: $1\n";
				print "$line\n";
			}
		}
#		if ($line =~ /([\xA1-\xDF]|[\x81-\x9F\xE0-\xEF][\x40-\x7E\x80-\xFC])/) {
#			print "$filename:$no: $1\n";
#			print "$line\n";
#		}
		$no++;
	}
	close(FP);
}

