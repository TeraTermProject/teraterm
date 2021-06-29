layer for unicode
=================

## layer for unicode について

Unicode API が存在しない環境で Unicode API を利用するためのライブラリ
- [Microsoft Layer for Unicode, MSLU](https://ja.wikipedia.org/wiki/Microsoft_Layer_for_Unicode)
- Windows 9x系を想定
- Unicode APIが利用できる場合は利用する
- Unicode APIが利用できない場合はANSI(非Unicode)APIを使用
  - Unicode API をエミュレーションする

## ビルド

### 必要なツール

- nasm
  - buildtools/ で `cmake -P nasm.cmake` を実行すると準備することができる
- perl
- make

### 準備

ビルド前に次のコマンドを実行しておく

```
perl generate.pl
make
```

## 経緯

- Tera Term では 9x系をサポートするため独自実装していた
  - Microsoft Layer for Unicode を使用すると dll が必要となる
  - 同梱するためにはライセンス、再頒布の検討が必要
  - dllが配布されなくなる可能性がある
  - Tera Term では独自実装のコードをバイナリにstaticにリンクしていた
- 従来 Tera Term の layer for unicode を使用するためには、少しコードを変更する必要があった
  - includeの追加
  - WIN32 API 名を少し変更する
    - 例 SetDlgItemTextW() -> _SetDlgItemTextW()
- ライブラリとして分離したい,モチベーション
  - 9x系のサポートを Tera Term 本体の開発と分離したい
  - Microsoft Layer for Unicode への置き換えを可能にしたい
  - x64(64bit)環境ではUnicode APIが存在するはずなので組み込まずにバイナリを作りたい
  - 将来9xサポートをやめるときにソースを修正を最小としたい
- [libunicodes](http://libunicows.sourceforge.net/)プロジェクトを参考にライブラリとして分離
  - Cでは記述できないのでアセンブラを使用
    - nasm を採用
      - linuxでも使用できる
    - ソースはテンプレートから自動生成
    - テンプレートを作成すれば他のアセンブラでも作成可能
  - ラベルに`@`が含まれるため
    - 例 `__imp__GetDlgItemTextW@16`

## 方針

- Tera Term が動作する程度に実装
- Microsoft Layer for Unicode と同等のAPI
  - `nm "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.xxxxxx.0\um\x86\unicows.lib"`
    - Visual Studio 2019 Win32 に含まれている
    - Visual Studio 2019 で作成したバイナリは 9x では動作しない
    - 従来のコードをそのままビルドできるよう,互換性を維持するため存在すると思われる

## ライブラリの利用

- 利用する側のソース
  - 利用するうえで特に気にかけることはない
- 利用する側のライブラリのリンク
  - kernel32.lib, user32.lib などよりも前に layer for unicode ライブラリをリンクする
    - 順序を誤ると Unicode API を使用するバイナリとなる
  - static リンクであれば、バイナリ内のAPIを置き換えることができる
    - CreateFileW() を使用していると思われる wfopen() も利用可能となる
- バイナリ内に Unicode API が存在するかチェックする例
  - 存在しない場合

        $ objdump -p ../../release/ttermpro.exe | grep CreateFile
                b1256      83  CreateFileA

  - 存在する場合

        $ objdump -p ../../release/ttermpro.exe | grep CreateFile
                b0618      83  CreateFileA
                b016c      86  CreateFileW
