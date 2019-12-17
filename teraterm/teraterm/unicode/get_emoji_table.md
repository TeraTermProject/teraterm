
# unicodeの絵文字

- unicode.orgにある 絵文字データ に基づいて絵文字のコード
  https://unicode.org/Public/emoji/12.0/emoji-data.txt
- 絵文字を判定するテーブルを作成する

# テーブルの作り方

- あらかじめ emoji-data.txt をダウンロードしておく
- 次のように実行
  - `perl get_emoji_table.pl > unicode_emoji.tbl`

# ?

- CJK環境では絵文字は全角
- 非CJKでは0x1f000未満の絵文字は半角、それ以外は全角 

## original data

https://unicode.org/Public/emoji/12.0/emoji-data.txt

  # emoji-data.txt
  # Date: 2019-01-15, 12:10:05 GMT
  # © 2019 Unicode®, Inc.
