# Susie Plug-in

Susie Plug-in は、画像ビュアーソフトSusieで様々な画像ファイルを読み込
むための外部ファイルです。Susie 以外でも利用されています。

Tera Term は Susie Plug-in を使用して
画像ファイルを読み込むことができます。

## ファイル

- libsusieplugin.cpp, libsusieplugin.h
  - Susie Plug-in を使用するソース
- tester/ (spi_tester.exe)
  - libsusieplugin テスト用プログラム
- plugin/
  - プラグインの実装

### プラグイン

TeraTerm Project Team によるサンプル実装。
Tera Term が利用するAPIのみ実装されています。

- ttspijpeg.dll
  - jpeg 用プラグイン
  - libjpeg-turbo を利用します
- ttspipng.dll
  - png 用プラグイン
  - libpng, zlib を利用します

# ビルド方法

- cmakeでビルドします
- プラグインのビルドには zlib, libjpeg-turbo, libpng が必要です
  - extlib で作成することができます

## vs2019 win32

例

    mkdir build_vs2019_win32
    cd build_vs2019_win32
    cmake -DEXTLIB=extlib/vs2019_win32 -G "Visual Studio 16 2019" -A Win32 ..
    cmake --build . --config Release

## vs2019 x64

例

    mkdir build_vs2019_x64
    cd build_vs2019_x64
    cmake -DEXTLIB=extlib/vs2019_x64 -G "Visual Studio 16 2019" -A x64 ..
    cmake --build . --config Release

# Susie plugin 仕様

次の情報源を参考にしました。

- Susieダウンロード
  - https://www.digitalpad.co.jp/~takechin/download.html
  - https://www.digitalpad.co.jp/~takechin/archives/splug004.lzh
    - PLUG_API.TXT

- Susie プラグイン API 定義ファイル
  - https://gist.github.com/tapetums/0a924712fb2fa0a7bb93

- TORO's Software library(Win32/Win64 Plugin)
  - http://toro.d.dooo.jp/slplugin.html
