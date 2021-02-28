# conv_combining.pl について

- hfs_plus.map (現在は uni_combining.map) を出力するために作成された
- conv_combining.pl の前は installer/conv_hfs.pl だった
- uni_combining.map は直接人手で修正している
  - https://ja.osdn.net/cvs/view/ttssh2/teraterm/source/teraterm/hfs_plus.map?hideattic=0&view=log
  - https://ja.osdn.net/projects/ttssh2/svn/view/trunk/teraterm/teraterm/uni_combining.map?root=ttssh2&view=log
  - r6514 にて hfs_plus.map -> uni_combining.map
- 現在はこのスクリプトの出力をそのまま使用していない

## 入力ファイル

- 現在の入手先
  - https://developer.apple.com/library/archive/technotes/tn/tn1150table.html

- 以前
  - スクリプト作成当時
    - UNICODE DECOMPOSITION TABLE.htm
  - 移動
    - http://developer.apple.com/technotes/tn/tn1150table.html

## 使い方

```
wget https://developer.apple.com/library/archive/technotes/tn/tn1150table.html
mv tn1150table.html "UNICODE DECOMPOSITION TABLE.htm"
perl conv_combining.pl > uni_combining.map
```
