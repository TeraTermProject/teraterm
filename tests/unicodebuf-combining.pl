# 結合文字、行末処理テスト

use utf8;
use Time::HiRes 'usleep';

binmode STDOUT, ":utf8";
local $| = 1;

$sleep_time = 100*1000;

# U+0041 + U+0302 (A + ^ → Â)
for ($i = 0; $i < 80; $i++) {
	print "\N{U+0041}";
	usleep($sleep_time);
	print "\N{U+0302}";
	usleep($sleep_time);
#	print "\N{U+0300}";
#	usleep($sleep_time);
}

# U+307B + U+309A (ほ + ゚ = ぽ)
for ($i = 0; $i < 40; $i++) {
	print "\N{U+307B}";
	usleep($sleep_time);
	if ((($i+1) % 40) == 0){ sleep(1); }
	print "\N{U+309A}";
	usleep($sleep_time);
	if ((($i+1) % 40) == 0){ sleep(1); }
}

# U+0061 U+0302 ( a+^ → â )
for ($i = 0; $i < 80; $i++) {
	print "\N{U+0061}";
	usleep($sleep_time);
	print "\N{U+0302}";
	usleep($sleep_time);
}
