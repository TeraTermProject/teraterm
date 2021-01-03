# conv_sjis.pl について

- uni2sjis.map を作るために作成された
- conv_sjis.pl の前は installer/conv.pl だった
- uni2sjis.map は直接人手で修正している
  - https://ja.osdn.net/cvs/view/ttssh2/teraterm/source/teraterm/uni2sjis.map?hideattic=0&view=log
  - https://ja.osdn.net/projects/ttssh2/svn/view/trunk/teraterm/teraterm/uni2sjis.map?root=ttssh2&view=log
- 現在はこのスクリプトの出力をそのまま使用していない

## 入力ファイル

- 最新はこれか?
  - ftp://ftp.unicode.org/Public/MAPPINGS/OBSOLETE/EASTASIA/JIS/SHIFTJIS.TXT
- 作成当時
  - SHIFTJIS_TXT.htm

## 使い方

```
wget ftp://ftp.unicode.org/Public/MAPPINGS/OBSOLETE/EASTASIA/JIS/SHIFTJIS.TXT
perl conv_sjis.pl > uni2sjis.map
```
