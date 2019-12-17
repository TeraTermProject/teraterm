
if not exist release\lang_utf16le mkdir release\lang_utf16le
perl utf8_to.pl utf16le release/lang_utf8/Default.lng release/lang_utf16le/Default.lng
perl utf8_to.pl utf16le release/lang_utf8/English.lng release/lang_utf16le/English.lng
perl utf8_to.pl utf16le release/lang_utf8/French.lng release/lang_utf16le/French.lng
perl utf8_to.pl utf16le release/lang_utf8/German.lng release/lang_utf16le/German.lng
perl utf8_to.pl utf16le release/lang_utf8/Japanese.lng release/lang_utf16le/Japanese.lng
perl utf8_to.pl utf16le release/lang_utf8/Korean.lng release/lang_utf16le/Korean.lng
perl utf8_to.pl utf16le release/lang_utf8/Russian.lng release/lang_utf16le/Russian.lng
perl utf8_to.pl utf16le "release/lang_utf8/Simplified Chinese.lng" "release/lang_utf16le/Simplified Chinese.lng"
perl utf8_to.pl utf16le "release/lang_utf8/Traditional Chinese.lng" "release/lang_utf16le/Traditional Chinese.lng"

if not exist release\lang mkdir release\lang
perl utf8_to.pl cp1252 release/lang_utf8/Default.lng release/lang/Default.lng
perl utf8_to.pl cp1252 release/lang_utf8/English.lng release/lang/English.lng
perl utf8_to.pl cp1252 release/lang_utf8/French.lng release/lang/French.lng
perl utf8_to.pl cp1252 release/lang_utf8/German.lng release/lang/German.lng
perl utf8_to.pl cp932 release/lang_utf8/Japanese.lng release/lang/Japanese.lng
perl utf8_to.pl cp949 release/lang_utf8/Korean.lng release/lang/Korean.lng
perl utf8_to.pl windows-1251 release/lang_utf8/Russian.lng release/lang/Russian.lng
perl utf8_to.pl cp936 "release/lang_utf8/Simplified Chinese.lng" "release/lang/Simplified Chinese.lng"
perl utf8_to.pl cp950 "release/lang_utf8/Traditional Chinese.lng" "release/lang/Traditional Chinese.lng"
