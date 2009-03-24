# 
# マクロコマンドのドキュメント類のチェックを行う
#
# [実行方法]
#   すべて(ttmparse.h より読み込み)のマクロを検証する
#   >perl macrotemplate.pl
#
#   指定したマクロを検証する
#   >perl macrotemplate.pl scpsend
#
# [改版履歴]
# 1.0 (2008.02.16 Yutaka Hirata)
# 1.1 (2008.02.23 Yutaka Hirata)
# 1.2 (2009.03.03 Yutaka Hirata)
# 1.3 (2009.03.04 NAGATA Shinya)
#

$macroidfile = '..\teraterm\ttpmacro\ttmparse.h';
$helpidfile = '..\teraterm\common\helpid.h';
$encmdfile = '..\doc\en\html\macro\command';
$jpcmdfile = '..\doc\jp\html\macro\command';
$enhhcfile = '..\doc\en\teraterm.hhc';
$jphhcfile = '..\doc\jp\teraterm.hhc';
$enhhpfile = '..\doc\en\teraterm.hhp';
$jphhpfile = '..\doc\jp\teraterm.hhp';
$keyfile = 'release\keyfile.ini';

if ($#ARGV != -1) {
	print "==== " . $ARGV[0] . "マクロを検証中...\n";
	do_main(lc($ARGV[0]));

} else {
	@FULL_COMMAND = ();
	open(FP, "$macroidfile") || die "Can't open $file.";
	while (<FP>) {
		if (/^#define Rsv(\w+)\s+(\d+)/) {
			if ($1 eq 'Operator') {
				last;
			}
			push(@FULL_COMMAND, "$1 $2");
		}
	}
	close(FP);
	
	foreach (@FULL_COMMAND) {
		if (/(\w+)\s+.*/) {
			$key = lc($1);
			
			print "==== $key マクロを検証中...\n";
			do_main($key);
		}
	}
}
exit(0);


#; TODO
#; en/teraterm.hhcにリンク挿入
#; en/teraterm.hhpにalias追加
#; jp/teraterm.hhcにリンク挿入
#; jp/teraterm.hhpにalias追加

sub do_main {
	my($macro) = @_;
	my($ret, $id, $s, $pat);
	my($idline, $helpline);
#	print "$macro\n";

	$s = "Rsv$macro\\b";
	$ret = read_keyword($macroidfile, $s);
	$idline = $ret;
#	print "$ret\n";
	if ($ret =~ /$macro\s+(\d+)/i) {
		$id = $1;
	} else {
		print "IDファイル($macroidfile)からマクロ定義が見つかりません\n";
		print "$idline\n";
		return;
	}
#	print "$id\n";

	# 置換
	# ttmparse.h と helpid.h で名前が一致していないため
	if ($macro eq 'if' ||
	    $macro eq 'then' ||
	    $macro eq 'else' ||
	    $macro eq 'elseif' ||
	    $macro eq 'endif'
	   ) {
		$macro = 'Ifthenelseif';
	}
	if ($macro eq 'for' ||
	    $macro eq 'next'
	   ) {
		$macro = 'fornext';
	}
	if ($macro eq 'endwhile'
	   ) {
		$macro = 'while';
	}
	if ($macro eq 'findfirst' ||
	    $macro eq 'findnext' ||
	    $macro eq 'findclose'
	   ) {
		$macro = 'Findoperations';
	}
	if ($macro eq 'millipause'
	   ) {
		$macro = 'mpause';
	}
	if ($macro eq 'rotatel'
	   ) {
		$macro = 'rotateleft';
	}
	if ($macro eq 'rotater'
	   ) {
		$macro = 'rotateright';
	}

	$s = "Command$macro\\b";
	$ret = read_keyword($helpidfile, $s);
	$helpline = $ret;
#	print "$ret\n";
	if ($ret =~ /$macro\s+(\d+)/i) {
		$n = 92000 + $id;
		if ($n != $1) {
			print "$helpidfile のIDが一致していません ($n != $1)\n";
			print "$idline\n";
			print "$ret\n";
			return;
		}
	} else {
		print "HELPIDファイル($helpidfile)からマクロ定義が見つかりません\n";
		print "$idline\n";
		print "$ret\n";
		return;
	}

	$pat = "\\b$macro\\b";
	$ret = read_keyword($keyfile, $pat);
	if ($ret eq '') {
		print "$keyfile ファイルに $pat コマンドがありません\n";
	}

	# 置換
	# helpid.h とコマンドのヘルプファイルで名前が一致していないため
	if ($macro eq 'do' ||
	    $macro eq 'loop'
	   ) {
		$macro = 'doloop';
	}
	if ($macro eq 'enduntil'
	   ) {
		$macro = 'until';
	}
	if ($macro eq 'crc32file'
	   ) {
		$macro = 'crc32';
	}
	
	$s = "$encmdfile\\$macro.html";
	if (!(-e $s)) {
		print "マクロコマンドの英語版説明文($s)がありません\n";
	}
	
	$s = "$jpcmdfile\\$macro.html";
	if (!(-e $s)) {
		print "マクロコマンドの日本語版説明文($s)がありません\n";
	}

	$s = "$encmdfile\\index.html";
	$pat = "$macro.html";
	$ret = read_keyword($s, $pat);
	if ($ret eq '') {
		print "$s ファイルに $pat へのリンクがありません\n";
	}
	
	$s = "$jpcmdfile\\index.html";
	$pat = "$macro.html";
	$ret = read_keyword($s, $pat);
	if ($ret eq '') {
		print "$s ファイルに $pat へのリンクがありません\n";
	}
	

	$pat = "$macro.html";
	$ret = read_keyword($enhhcfile, $pat);
	if ($ret eq '') {
		print "$enhhcfile ファイルに $pat へのリンクがありません\n";
	}

	$pat = "$macro.html";
	$ret = read_keyword($jphhcfile, $pat);
	if ($ret eq '') {
		print "$jphhcfile ファイルに $pat へのリンクがありません\n";
	}
	
	
	$pat = "$macro.html";
	$ret = read_keyword($enhhpfile, $pat);
	if ($ret eq '') {
		print "$enhhpfile ファイルに $pat へのALIASリンクがありません\n";
	}

	$pat = "$macro.html";
	$ret = read_keyword($jphhpfile, $pat);
	if ($ret eq '') {
		print "$jphhpfile ファイルに $pat へのALIASリンクがありません\n";
	}

}

sub read_keyword {
	my($file, $keyword) = @_;
	my($line) = '';
	my($found) = 0;
	
	open(FP, "$file") || die "Can't open $file.";
	while (<FP>) {
		chomp;
		$line = $_;
		if (/$keyword/i) {
			$found = 1;
			last;
		}
	}
	close(FP);
	
	if ($found == 0) {
		$line = '';
	}
	return ($line);
}
