
# icoファイルの保守

保守の流れ

1. 準備する  
   ツールを用意する
2. ico(アイコン)ファイルファイルから個別の画像を抽出する
3. ファイルの入れ替えや編集を行う
4. 個別の画像から ico ファイルを生成する


- ツールの準備 icotool
- icoファイルから複数のpngを抽出
- 複数のpngから1つのicoファイルを合成

## 準備

- icotoolを使える状態にする
- cygwin では icoutils パッケージに入っている
- linux では icoutils パッケージに入っている
  - `apt-get install icoutils`
- windows用も存在する

## icoファイルからpngの抽出

- icon_extract.shを実行する
- pngファイルが各フォルダに抽出される

## pngからicoファイルを合成

- icon_conbine.shを実行する
- pngファイルからicoファイルが合成される
- icoファイルはこのフォルダに作成される
- 必要に応じて../*.icoと入れ替えを行う
