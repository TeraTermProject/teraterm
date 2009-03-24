# 
# 言語ファイルのチェックを行う
#
# [実行方法]
#   >perl langfile.pl
#
# [改版履歴]
# 1.0 (2008.02.23 Yutaka Hirata)
#

$langfile = '..\release\lang\*.lng';
$secpattern = '^\[(.+)\]';

while (glob($langfile)) {
	do_main($_);
#	last;
}

exit(0);

sub do_main {
	my($file) = @_;
	local(@lines, $pos, %hash);
	my($section);
	
	print "===== $file を検証中\n";
	open(FP, $file) || die;
	@lines = <FP>;
	$pos = 0;
	close(FP);
	
#	print @lines;

	print "使用禁止ショートカットを検索中\n";
	do {
		$section = read_section();
		print "$section セクション\n";
		read_entry();
	} while ($pos < @lines);
	
	print "ショートカットの重複を検索中\n";
	check_conflict();

}

sub read_section {
	my($line, $i, $s);
	
	$s = '';
	for ($i = $pos ; $i < @lines ; $i++) {
#		chomp($lines[$i]);
		if ($lines[$i] =~ /$secpattern/) {
			$s = $1;
#			print "Section: $s\n";
			$pos = $i + 1;
			last;
		}
	}
	return ($s);
}

sub read_entry {
	my($line, $i, $s);
	my($id, $val);
	
	for ($i = $pos ; $i < @lines ; $i++) {
#		chomp($lines[$i]);
#		print "$i: $lines[$i]";
		if ($lines[$i] =~ /^[\s\n]+$/ || 
			$lines[$i] =~ /^\s*;/
			) {
			# ignore
#			print "無視\n";
			
		} elsif ($lines[$i] =~ /$secpattern/) {
			last;
			
		} elsif ($lines[$i] =~ /(\w+)\s*=\s*(.+)/) {
			$id = $1;
			$val = $2;
#			print "$id と $val\n";
			
			# トップメニューのチェック
			if ($id eq 'MENU_FILE' ||
				$id eq 'MENU_EDIT' ||
				$id eq 'MENU_SETUP' ||
				$id eq 'MENU_CONTROL' ||
				$id eq 'MENU_WINDOW' ||
				$id eq 'MENU_HELP' ||
				$id eq 'MENU_KANJI') {
				if (check_invalid_key($val)) {
					print "$id エントリの $val には使用禁止のショートカットキーがあります\n";
				}
			}
			
			# ハッシュへ登録
			$hash{$id} = $val;
			
		} else {
			print "Unknown error.\n";
		}
	}
	
	$pos = $i;
}

sub check_invalid_key {
	my($arg) = @_;
	my($keys) = "TCIQNVRPBGD";
	my($key);
	
	if ($arg =~ /&(\w)/) {
		$key = uc($1);
		if (index($keys, $key) != -1) {  # NG!
			return 1;
		}
	}
	return 0;  # safe
}

sub check_conflict {
	my($line, @lines2);
	my($section, $key, $val);
	my($line2, $shortcut, $samelevel);
	my($key2, $val2);
	
	@lines2 = @lines;
	foreach $line (@lines) {
		if ($line =~ /^\[(.+)\]$/) {
			$section = $1;
			next;
		}
		elsif ($line =~ /^(.+)=(.+)$/) {
			$key = $1;
			$val = $2;
			if ($val =~ /&(\w)/) {
				$shortcut = $1;
				$samelevel = $key;
				$samelevel =~ s/(\w+)_[a-zA-Z0-9]+/\1/;
				# print "$key $samelevel $shortcut\n";
				foreach $line2 (@lines2) {
					if ($line2 =~ /^\[(.+)\]$/) {
						$section2 = $1;
						next;
					}
					if ($section ne $section2) {
						next;
					}
					elsif ($line2 =~ /^(${samelevel}_[a-zA-Z0-9]+)=(.+)$/) {
						$key2 = $1;
						$val2 = $2;
						if ($key2 eq $key) {
							next;
						}
						if ($val2 =~ /&$shortcut/i) {
							print "[$key=$val] and [$key2=$val2] conflict [&$shortcut]\n";
						}
					}
				}
			}
		}
	}

}
