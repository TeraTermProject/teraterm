
# unicodeの絵文字

- unicode.orgにある 絵文字データ に基づいて絵文字のコード
  - https://www.unicode.org/Public/UCD/latest/ucd/emoji/emoji-data.txt
- 絵文字を判定するテーブルを作成する
  - unicode_emoji.tbl

# etc

- CJK環境では絵文字は全角
- 非CJKでは0x1f000未満の絵文字は半角、それ以外は全角

# テーブルの作り方

- あらかじめ emoji-data.txt をダウンロードしておく
  - `wget https://www.unicode.org/Public/UCD/latest/ucd/emoji/emoji-data.txt`
- 次のように実行
  - `perl get_emoji_table.pl`

実行例
```
wget https://www.unicode.org/Public/UCD/latest/ucd/emoji/emoji-data.txt
perl get_emoji_table.pl
```
