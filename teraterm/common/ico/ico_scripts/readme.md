
# icoファイルの保守

保守の流れ

1. 準備する  
   ツールを用意する
2. ico(アイコン)ファイルファイルから個別の画像を抽出する
3. ファイルの入れ替えや編集を行う
4. 個別の画像から ico ファイルを生成する

## 準備

- icotoolを使える状態にする
  - icotool についてを参照

## icoファイルからpngの抽出

- icon_extract.cmakeを実行する
- 処理したicoファイルと同名のフォルダが作成され
- pngファイルが各フォルダに抽出される

## pngからicoファイルを合成

- icon_conbine.cmakeを実行する
- pngファイルからicoファイルが合成される
- icoファイルはこのフォルダに作成される
- 次のオプションでicoファイルのコピーを行う
  - `-DCOPY_ICO=1`
    - icoファイルを元ファイルに上書きする
  - `-DCOPY_BMP_ICO=1`
    - bmp形式のみのicoファイルを元ファイルのフォルダにコピーする
    - ファイル名は `元ファイル名_bmp.ico`

## ico ファイルについて

- [プロジェクトページのアイコンについて](https://osdn.net/projects/ttssh2/wiki/%E3%82%A2%E3%82%A4%E3%82%B3%E3%83%B3)
- *注意点* Visual Studio 2005 では png形式は扱えない
  - bmp形式にしておく必要がある

## icotool について

- https://www.nongnu.org/icoutils/
- icoファイルから複数のpngを抽出
- 複数のpngから1つのicoファイルを合成
- windows用
  - https://www.cybercircuits.co.nz/web/blog/icoutils-0-32-3-for-windows
- cygwin では icoutils パッケージに入っている
- linux では icoutils パッケージに入っている
  - `apt-get install icoutils`
