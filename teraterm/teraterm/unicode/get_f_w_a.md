
# unicodeの全角半角

- unicode.orgにある East Asian Width に基づいて決める

# テーブルの作り方

- あらかじめ EastAsianWidth.txt をダウンロードしておく
- 次のように実行
  - `perl get_f_w_a.pl > unicode_asian_width.tbl`

## East Asian Width について

https://ja.wikipedia.org/wiki/%E6%9D%B1%E3%82%A2%E3%82%B8%E3%82%A2%E3%81%AE%E6%96%87%E5%AD%97%E5%B9%85

## original data

http://www.unicode.org/Public/UCD/latest/ucd/EastAsianWidth.txt

    EastAsianWidth-12.1.0.txt
    Date: 2019-03-31, 22:01:58 GMT [KW, LI]
