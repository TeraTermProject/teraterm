#! /usr/bin/perl

#
# 英語版ドキュメントに日本語が含まれていないかを調べる。
#
# Usage(ActivePerl):
#  perl check_sjis_code.pl > result.txt
#

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
		next if( $path =~ /^\.svn$/ );                # '.svn' はスキップ
		
		my $full_path = "$top_dir" . '/' . "$path";
		next if (-B $full_path);     # バイナリファイルはスキップ
		
#		print "$full_path\r\n";                     # 表示だけなら全てを表示してくれる-------
		push(@paths, $full_path);                       # データとして取り込んでも前の取り込みが初期化される
		if( -d "$top_dir/$path" ){                      #-- ディレクトリの場合は自分自身を呼び出す
			&get_file_paths("$full_path");
			
		} else {
			check_sjis_code($full_path);
		
		}
	}
	return \@paths;
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
		if ($line =~ /([\xA1-\xDF]|[\x81-\x9F\xE0-\xEF][\x40-\x7E\x80-\xFC])/) {
			print "$filename:$no: $1\n";
			print "$line\n";
		}
		$no++;
	}
	close(FP);
}

