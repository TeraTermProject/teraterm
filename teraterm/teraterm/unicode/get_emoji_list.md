
# Unicode の絵文字の East Asian Width

文字ごとの文字幅を一覧にしたテキストファイルを出力する

## テキストファイルの作り方

- あらかじめ emoji-data.txt をダウンロードしておく
  - https://www.unicode.org/Public/UCD/latest/ucd/emoji/emoji-data.txt
- あらかじめ EastAsianWidth.txt をダウンロードしておく
  https://www.unicode.org/Public/UCD/latest/ucd/EastAsianWidth.txt
- 次のように実行
  - `perl get_emoji_list.pl`

実行例
```
wget https://www.unicode.org/Public/UCD/latest/ucd/emoji/emoji-data.txt
wget https://www.unicode.org/Public/UCD/latest/ucd/EastAsianWidth.txt
perl get_emoji_list.pl
```
