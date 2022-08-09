#
# copyright の年の更新を補助するためのスクリプト
#	注意
#		このスクリプトを実行すると
#		ファイル内のすべての年号を書き換えるが
#		変更されたモジュール(ttermproなど)だけを更新して
#		コミットすること
#
use strict;
use warnings;
use utf8;

my $year_from = "2020";
my $year_to = "2022";

my @file_list = (
	"TTProxy/TTProxy.rc",
	"TTXKanjiMenu/ttxkanjimenu.rc",
	"doc/en/html/about/copyright.html",
	"doc/en/html/index.html",
	"doc/en/html/macro/index.html",
	"doc/ja/html/about/copyright.html",
	"doc/ja/html/index.html",
	"doc/ja/html/macro/index.html",
	"installer/release/license.txt",
	"installer/teraterm.iss",
	"installer/teraterm_cmake.iss.in",
	"teraterm/keycode/keycode-version.rc",
	"teraterm/teraterm/tt-version.rc",
	"teraterm/ttpcmn/ttpcmn-version.rc",
	"teraterm/ttpdlg/ttpdlg.rc",
	"teraterm/ttpmacro/ttm-version.rc",
	"teraterm/ttptek/ttptek-version.rc",
	"ttpmenu/ttpmenu.rc",
	"ttssh2/ttxssh/ttxssh-version.rc",
	"ttssh2/ttxssh/ttxssh.rc",
	);

sub replace_year
{
	my ($file) = @_;

	# すべて読み込む
	open(FD, "<$file");
	my @content = <FD> ;
	close(FD) ;

	my $bak = $file . ".bak";
	unlink($bak);
	rename($file, $bak) || die "error $bak $!";

	# 上書きで書き出す
	my $out = $file;
	open(FD , ">$out" ) || die "error $out";
	foreach(@content) {
		my $line = $_ ;
		$line =~ s/$year_from/$year_to/g ;
		print FD "$line" ;
	}
	close(FD) ;
}

foreach my $file (@file_list) {
	$file = "../" . $file;
	replace_year($file);
}
