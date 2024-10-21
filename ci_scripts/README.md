# CI用スクリプト

- AppVeyor用スクリプト
- ローカルでビルド
  - AppVeyor用スクリプトをローカルでテスト
  - これを使えば簡単にビルドすることができる

## 準備

あらかじめ次のツールをインストールしておく

- Visual Studio 2022
  - arm64をビルドするときは次のパッケージを入れる
    - `MSVC v143 - VS 2022 C++ ARM64/ARM64EC ビルド ツール (最新)`
- MinGW
  - c:\msys64 を削除
  - https://www.msys2.org/ からインストーラをダウンロード
  - インストール

## ビルド

- `ci_scripts\build_local_appveyor.bat` を実行

## 再度ビルドするとき

### Visual Studioの場合

- ビルドフォルダの teraterm_all.sln をダブルクリック

### MinGWの場合

- スタートから MSYS2 / MSYS2 MINGW64 を起動
  - win32系をビルドするときは MINGW32 を起動
- ビルドフォルダへ移動
  - `cd ..../build_mingw_x64_clang_msys2`
- `make`
  - `make -j 4` などで複数プロセスでビルド
