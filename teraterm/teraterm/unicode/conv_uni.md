# conv_uni.pl について

- sjis2uni.map を作るために作成されたと思われる
- conv_uni.pl の前は installer/rev_conv.pl だった
- sjis2uni.map はソースツリーに存在しないし、使用されていない

## 入力ファイル

- 最新はこれか?
  - ftp://ftp.unicode.org/Public/MAPPINGS/OBSOLETE/EASTASIA/JIS/SHIFTJIS.TXT
- 作成当時
  - SHIFTJIS_TXT.htm

## 使い方

```
wget ftp://ftp.unicode.org/Public/MAPPINGS/OBSOLETE/EASTASIA/JIS/SHIFTJIS.TXT
perl conv_uni.pl > sjis2uni.map
```
