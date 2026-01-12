# shift jis

- 元データ
  - http://www.unicode.org/Public/MAPPINGS/OBSOLETE/EASTASIA/JIS/SHIFTJIS.TXT

コードポイントU+0080以上で、Shift-JISの文字を全て2cell(全角)として扱う
iniファイルの一部を作成する。

iniファイルはUTF-8で作成される。

# iniファイルの作り方

```
wget http://www.unicode.org/Public/MAPPINGS/OBSOLETE/EASTASIA/JIS/SHIFTJIS.TXT
perl shiftjis.pl shiftji.ini
```
