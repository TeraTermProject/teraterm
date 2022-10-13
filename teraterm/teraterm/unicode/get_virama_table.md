# virama charactors

- 結合文字
- 次の文字を、前の文字とくっつける

- 元情報 unicode.org
  - https://www.unicode.org/reports/tr44/#UnicodeData.txt
- UnicodeData.txt の Canonical_Combining_Class を使ってテーブルを作成する
  - https://www.unicode.org/reports/tr44/#Canonical_Combining_Class
- UnicodeData.txt
  - https://www.unicode.org/Public/UCD/latest/ucd/UnicodeData.txt
- 元データ(UnicodeData.txt)のversion
  - バージョンは次のファイルに書いてある
  - https://www.unicode.org/Public/UCD/latest/ucd/ReadMe.txt
  - Version 15.0.0 of the Unicode Standard.

# テーブルの作り方

- UnicodeData.txt をダウンロード
- スクリプトを実行
- unicode_combine.tbl が出力される

実行例
```
wget https://www.unicode.org/Public/UCD/latest/ucd/UnicodeData.txt
perl get_virama_table.pl
```

