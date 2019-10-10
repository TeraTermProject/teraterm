# ansi -> utf-8
mkdir utf8
echo -ne '\xEF\xBB\xBF' > utf8/default.lng
iconv -c -t utf-8 -f CP1252 default.lng >> utf8/default.lng
echo -ne '\xEF\xBB\xBF' > utf8/english.lng
iconv -c -t utf-8 -f CP1252 english.lng >> utf8/english.lng
echo -ne '\xEF\xBB\xBF' > utf8/French.lng
iconv -c -t utf-8 -f CP1252 French.lng >> utf8/French.lng
echo -ne '\xEF\xBB\xBF' > utf8/German.lng
iconv -c -t utf-8 -f CP1252 German.lng >> utf8/German.lng
echo -ne '\xEF\xBB\xBF' > utf8/Japanese.lng
iconv -c -t utf-8 -f CP932  Japanese.lng >> utf8/Japanese.lng
echo -ne '\xEF\xBB\xBF' > utf8/Korean.lng
iconv -c -t utf-8 -f CP949  Korean.lng >> utf8/Korean.lng
echo -ne '\xEF\xBB\xBF' > utf8/Russian.lng
iconv -c -t utf-8 -f windows-1251  Russian.lng >> utf8/Russian.lng
echo -ne '\xEF\xBB\xBF' > "utf8/Simplified Chinese.lng"
iconv -c -t utf-8 -f CP936  "Simplified Chinese.lng" >> "utf8/Simplified Chinese.lng"
echo -ne '\xEF\xBB\xBF' > "utf8/Traditional Chinese.lng"
iconv -c -t utf-8 -f CP950  "Traditional Chinese.lng" >> "utf8/Traditional Chinese.lng"

# utf-8 -> ansi
mkdir ansi
iconv -c -f utf-8 -t CP1252 utf8/default.lng > ansi/default.lng
iconv -c -f utf-8 -t CP1252 utf8/english.lng > ansi/english.lng
iconv -c -f utf-8 -t CP1252 utf8/French.lng > ansi/French.lng
iconv -c -f utf-8 -t CP1252 utf8/German.lng > ansi/German.lng
iconv -c -f utf-8 -t CP932  utf8/Japanese.lng > ansi/Japanese.lng
iconv -c -f utf-8 -t CP949  utf8/Korean.lng > ansi/Korean.lng
iconv -c -f utf-8 -t windows-1251  utf8/Russian.lng  > ansi/Russian.lng
iconv -c -f utf-8 -t CP936  "utf8/Simplified Chinese.lng" > "ansi/Simplified Chinese.lng"
iconv -c -f utf-8 -t CP950  "utf8/Traditional Chinese.lng" > "ansi/Traditional Chinese.lng"

# utf-8 -> utf-16le
mkdir utf16le
iconv -c -f utf-8 -t utf-16le utf8/default.lng > utf16le/default.lng
iconv -c -f utf-8 -t utf-16le utf8/english.lng > utf16le/english.lng
iconv -c -f utf-8 -t utf-16le utf8/French.lng > utf16le/French.lng
iconv -c -f utf-8 -t utf-16le utf8/German.lng > utf16le/German.lng
iconv -c -f utf-8 -t utf-16le utf8/Japanese.lng > utf16le/Japanese.lng
iconv -c -f utf-8 -t utf-16le utf8/Korean.lng > utf16le/Korean.lng
iconv -c -f utf-8 -t utf-16le utf8/Russian.lng  > utf16le/Russian.lng
iconv -c -f utf-8 -t utf-16le "utf8/Simplified Chinese.lng" > "utf16le/Simplified Chinese.lng"
iconv -c -f utf-8 -t utf-16le "utf8/Traditional Chinese.lng" > "utf16le/Traditional Chinese.lng"
