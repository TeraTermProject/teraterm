
# unicodeの全角半角

- unicode.orgにある East Asian Width に基づいて決める
  - https://www.unicode.org/Public/UCD/latest/ucd/EastAsianWidth.txt
- `unicode_asian_width.tbl` を生成する

## East Asian Width について

https://ja.wikipedia.org/wiki/%E6%9D%B1%E3%82%A2%E3%82%B8%E3%82%A2%E3%81%AE%E6%96%87%E5%AD%97%E5%B9%85

# テーブルの作り方

- あらかじめ EastAsianWidth.txt をダウンロードしておく
  - `wget https://www.unicode.org/Public/UCD/latest/ucd/EastAsianWidth.txt`
- 次のように実行
  - `perl get_asianwidth_table.pl`

実行例
```
wget https://www.unicode.org/Public/UCD/latest/ucd/EastAsianWidth.txt
perl get_asianwidth_table.pl
```
