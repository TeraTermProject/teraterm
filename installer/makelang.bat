
if not exist release\lang_utf16le mkdir release\lang_utf16le
perl utf8_to.pl utf16le release/lang_utf8/Default.lng release/lang_utf16le/Default.lng
perl utf8_to.pl utf16le release/lang_utf8/en_US.lng release/lang_utf16le/en_US.lng
perl utf8_to.pl utf16le release/lang_utf8/fr_FR.lng release/lang_utf16le/fr_FR.lng
perl utf8_to.pl utf16le release/lang_utf8/de_DE.lng release/lang_utf16le/de_DE.lng
perl utf8_to.pl utf16le release/lang_utf8/ja_JP.lng release/lang_utf16le/ja_JP.lng
perl utf8_to.pl utf16le release/lang_utf8/ko_KR.lng release/lang_utf16le/ko_KR.lng
perl utf8_to.pl utf16le release/lang_utf8/ru_RU.lng release/lang_utf16le/ru_RU.lng
perl utf8_to.pl utf16le "release/lang_utf8/zh_CN.lng" "release/lang_utf16le/zh_CN.lng"
perl utf8_to.pl utf16le release/lang_utf8/es_ES.lng release/lang_utf16le/es_ES.lng
perl utf8_to.pl utf16le "release/lang_utf8/zh_TW.lng" "release/lang_utf16le/zh_TW.lng"
perl utf8_to.pl utf16le "release/lang_utf8/ta_IN.lng" "release/lang_utf16le/ta_IN.lng"
perl utf8_to.pl utf16le "release/lang_utf8/pt_BR.lng" "release/lang_utf16le/pt_BR.lng"
perl utf8_to.pl utf16le "release/lang_utf8/it_IT.lng" "release/lang_utf16le/it_IT.lng"
perl utf8_to.pl utf16le "release/lang_utf8/tr_TR.lng" "release/lang_utf16le/tr_TR.lng"

if not exist release\lang mkdir release\lang
perl utf8_to.pl cp1252 release/lang_utf8/Default.lng release/lang/Default.lng
perl utf8_to.pl cp1252 release/lang_utf8/en_US.lng release/lang/en_US.lng
perl utf8_to.pl cp1252 release/lang_utf8/fr_FR.lng release/lang/fr_FR.lng
perl utf8_to.pl cp1252 release/lang_utf8/de_DE.lng release/lang/de_DE.lng
perl utf8_to.pl cp932 release/lang_utf8/ja_JP.lng release/lang/ja_JP.lng
perl utf8_to.pl cp949 release/lang_utf8/ko_KR.lng release/lang/ko_KR.lng
perl utf8_to.pl windows-1251 release/lang_utf8/ru_RU.lng release/lang/ru_RU.lng
perl utf8_to.pl cp936 "release/lang_utf8/zh_CN.lng" "release/lang/zh_CN.lng"
perl utf8_to.pl cp1252 release/lang_utf8/es_ES.lng release/lang/es_ES.lng
perl utf8_to.pl cp950 "release/lang_utf8/zh_TW.lng" "release/lang/zh_TW.lng"
perl utf8_to.pl cp65001 release/lang_utf8/ta_IN.lng release/lang/ta_IN.lng
perl utf8_to.pl cp1252 "release/lang_utf8/pt_BR.lng" "release/lang/pt_BR.lng"
perl utf8_to.pl cp1252 "release/lang_utf8/it_IT.lng" "release/lang/it_IT.lng"
perl utf8_to.pl cp1254 "release/lang_utf8/tr_TR.lng" "release/lang/tr_TR.lng"
