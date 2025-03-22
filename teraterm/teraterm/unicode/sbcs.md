# SBCS Code Page

[Character encoding mappings and related files @ unicode.org][1]

[1]: https://www.unicode.org/L2/L1999/99325-E.htm

# original data

- 次のようにして取得

```
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/PC/CP437.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/PC/CP737.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/PC/CP775.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/PC/CP850.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/PC/CP852.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/PC/CP855.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/PC/CP857.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/PC/CP860.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/PC/CP861.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/PC/CP862.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/PC/CP863.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/PC/CP864.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/PC/CP865.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/PC/CP866.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/PC/CP869.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/PC/CP874.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1250.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1251.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1252.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1253.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1254.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1255.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1256.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1257.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1258.TXT
wget https://unicode.org/Public/MAPPINGS/VENDORS/MISC/KOI8-R.TXT
```

# create table

- 次のコマンドを実行する

```
perl iso8859.pl CP437.TXT cp437.map
perl iso8859.pl CP737.TXT cp737.map
perl iso8859.pl CP775.TXT cp775.map
perl iso8859.pl CP850.TXT cp850.map
perl iso8859.pl CP852.TXT cp852.map
perl iso8859.pl CP855.TXT cp855.map
perl iso8859.pl CP857.TXT cp857.map
perl iso8859.pl CP860.TXT cp860.map
perl iso8859.pl CP861.TXT cp861.map
perl iso8859.pl CP862.TXT cp862.map
perl iso8859.pl CP863.TXT cp863.map
perl iso8859.pl CP864.TXT cp864.map
perl iso8859.pl CP865.TXT cp865.map
perl iso8859.pl CP866.TXT cp866.map
perl iso8859.pl CP869.TXT cp869.map
perl iso8859.pl CP874.TXT cp874.map
perl iso8859.pl CP1250.TXT cp1250.map
perl iso8859.pl CP1251.TXT cp1251.map
perl iso8859.pl CP1252.TXT cp1252.map
perl iso8859.pl CP1253.TXT cp1253.map
perl iso8859.pl CP1254.TXT cp1254.map
perl iso8859.pl CP1255.TXT cp1255.map
perl iso8859.pl CP1256.TXT cp1256.map
perl iso8859.pl CP1257.TXT cp1257.map
perl iso8859.pl CP1258.TXT cp1258.map
perl iso8859.pl CP1258.TXT cp1258.map
perl iso8859.pl KOI8-R.TXT koi8-r.map
```
