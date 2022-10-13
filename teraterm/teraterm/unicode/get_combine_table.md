# combining charactors

- 結合文字
- 濁点など、前のもじとくっつける処理
  https://en.m.wikipedia.org/wiki/Combining_character
- 文字の幅が広がる/広がらないの2種類
  - Nonspacing_Mark 	a nonspacing combining mark (zero advance width)
    - EMOJI MODIFIER
    - VARIATION SELECTOR
    - Enclosing_Mark
  - Spacing_Mark    	a spacing combining mark (positive advance width)

- 元情報 unicode.org
  - https://www.unicode.org/reports/tr44/#UnicodeData.txt
- UnicodeData.txt の General_Category を使ってテーブルを作成する
  - https://www.unicode.org/reports/tr44/#General_Category
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
perl get_combine_table.pl
```

