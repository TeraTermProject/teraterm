# unicode の block

- 元情報 unicode.org
  - https://www.unicode.org/reports/tr44/#Blocks.txt

# テーブルの作り方

- Blocks.txt をダウンロード
- スクリプトを実行
- unicode_block.tbl が出力される

実行例
```
wget https://www.unicode.org/Public/UCD/latest/ucd/Blocks.txt
perl get_block_table.pl
```
