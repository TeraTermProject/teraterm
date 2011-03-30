#! /usr/bin/perl

#
# HTMLヘルプのインデックスファイルを生成する
#
# Usage(ActivePerl):
#  perl htmlhelp_index_make.pl
#

use Cwd;
@dirstack = (); 

do_main($ARGV[0], $ARGV[1]);

exit(0);

sub do_main {
	my($path, $body) = @_;

	print << 'EOD';
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<HTML><HEAD>
<meta name="GENERATOR" content="Tera Term Project">
<!-- Sitemap 1.0 -->
</HEAD><BODY>
<UL>
EOD

	push @dirstack, getcwd; 
	chdir $path; 
	get_file_paths($body);
	chdir pop @dirstack; 

	print << 'EOD';
</UL>
</BODY></HTML>
EOD

}


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
			check_html_file($full_path);
		
		}
	}
	return \@paths;
}

sub check_html_file {
	my($filename) = shift;
	local(*FP);
	my($line, $no);
	
	if ($filename !~ /.html$/) {
		return;
	}
	
	open(FP, "<$filename") || return;
	$no = 1;
	while ($line = <FP>) {
#		$line = chomp($line);
#		print "$line\n";
		if ($line =~ /<TITLE>(.+)<\/TITLE>/i) {
#			print "$filename:$no: $1\n";
#			print "$line\n";
			write_add_index($filename, $1);
			last;
		}

		$no++;
	}
	close(FP);
}

sub write_add_index {
	my($filename, $title) = @_;
	
	print << "EOD";
<LI><OBJECT type="text/sitemap">
<param name="Name" value="$title">
<param name="Local" value="$filename">
</OBJECT>
EOD

}

