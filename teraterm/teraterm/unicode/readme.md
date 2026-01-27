# 文字コード関連のマップファイル,スクリプトファイルについて

## [uni_combining.map](../uni_combining.map)

- NFDで分解されている文字を結合するために使用
```
		// WideCharToMultiByte() では結合処理は行われない
		// 自力で結合処理を行う。ただし、最初の2文字だけ
		//	 例1:
		//		U+307B(ほ) + U+309A(゜) は
		//		Shift jis の 0x82d9(ほ) と 0x814b(゜) に変換され
		//		0x82db(ぽ) には変換されない
		//		予め U+307D(ぽ)に正規化しておく
```
- [conv_combining.md](conv_combining.md)

## [uni2sjis.map](../uni2sjis.map)

- UnicodeからShift JISに変換するために使用
- [conv_sjis.md](conv_sjis.md)

## sjis2uni.map はソースツリーに存在しない

- Shift JISからUnicodeからに変換できると思われるが、使用されていない
- [conv_uni.md](conv_uni.md)

## [unisym2decsp.map](../unisym2decsp.map)

- UnicodeからDEC special文字コードに変換するために使用

## [unicode_asian_width.tbl](../unicode_asian_width.tbl)

- East Asian Width 特性テーブル
  - [東アジアの文字幅(wikiepdia)](https://ja.wikipedia.org/wiki/%E6%9D%B1%E3%82%A2%E3%82%B8%E3%82%A2%E3%81%AE%E6%96%87%E5%AD%97%E5%B9%85)参照
- [get_asianwidth_table.md](get_asianwidth_table.md)

## [unicode_combine.tbl](../unicode_combine.tbl)

- combine (結合)文字かどうかを調べるテーブル
- [get_combine_table.md](get_combine_table.md)

## [unicode_emoji.tbl](../unicode_emoji.tbl)

- 絵文字判定のためのテーブル
- [get_emoji_table.md](get_emoji_table.md)

## [unicode_virama.tbl](../unicode_virama.tbl)

- ヴィラーマ判定のためのテーブル
- [get_virama_table.md](get_virama_table.md)

## [unicode_block.tbl](../unicode_block.tbl)

- Unicode block のテーブル
- [get_block_table.md](get_block_table.md)

## iso8859-X.md

- [iso8859.md](iso8859.md)
- 2部(iso8859-2.tbl)から16部(iso8859-16.tbl)まで
  - 12部は規格上破棄されていて存在しない

## shiftjis.md

- [shiftjis.md](shiftjis.md)

## jis0208.md

- [jis0208.md](jis0208.md)

## UnicodeXX.0-emoji.txt

- Unicode 絵文字の文字幅を一覧にしたテキストファイル
- プログラムからは利用しない
- [get_emoji_list.md](get_emoji_list.md)

