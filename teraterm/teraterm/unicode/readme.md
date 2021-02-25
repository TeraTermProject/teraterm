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
- [get_f_w_a.md](get_f_w_a.md)

## [unicode_combine.tbl](../unicode_combine.tbl)

- combine (結合)文字かどうかを調べるテーブル
- [get_combine_table.md](get_combine_table.md)

## [unicode_emoji.tbl](../unicode_emoji.tbl)

- 絵文字判定のためのテーブル
- [get_emoji_table.md](get_emoji_table.md)
