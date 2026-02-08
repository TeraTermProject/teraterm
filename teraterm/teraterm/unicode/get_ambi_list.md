
# Unicode Ambiguous 文字

Ambiguous 文字を一覧にしたテキストファイルを出力する

## テキストファイルの作り方

- あらかじめ EastAsianWidth.txt をダウンロードしておく
  https://www.unicode.org/Public/UCD/latest/ucd/EastAsianWidth.txt
- あらかじめ emoji-data.txt をダウンロードしておく
  - https://www.unicode.org/Public/UCD/latest/ucd/emoji/emoji-data.txt
- 次のように実行
  - `perl get_ambi_list.pl`

実行例
```
wget https://www.unicode.org/Public/UCD/latest/ucd/EastAsianWidth.txt
wget https://www.unicode.org/Public/UCD/latest/ucd/emoji/emoji-data.txt
perl get_ambi_list.pl
```
