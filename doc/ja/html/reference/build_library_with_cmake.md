
# libsフォルダ

- Tera Term のビルドに利用する外部のライブラリを置いておくためのフォルダ
- 各コンパイラ向けにソース/ライブラリ/実行ファイルを置いておく
- ライブラリはあらかじめ1度だけ生成しておく

# 準備

## Visual Studio

- cmake
  - PATHが通してあればok
  - Cygwinのcmakeはつかえない(Visual Studioをサポートしていない)
  - Visual Studio 2005 を使う場合は cmake 3.11.4 を使用する必要がある
- perl
  - ドキュメントファイルの文字コード・改行コード変換に必要
  - OpenSSL のコンパイル (Tera Term 5は OpenSSL から LibreSSL に切り替えたため使用していない)
  - ActivePerl 5.8 以上、または cygwin perl
  - PATHが通っていなければ自動で探す

## MinGW 共通 (experimental)

- Cygwin,MSYS2,linux(wsl)上のMinGWでビルド可能
- 各環境で動作するcmake,make,(MinGW)gcc,(clang)が必要

# ビルド手順

必要なアーカイブを自動的にダウンロードするので、
インターネットが利用できる環境でビルドする必要がある

## Visual Studioの場合

### batファイルを使用する場合

libs/buildall_cmake.bat を実行して使用する Visual Studioを選ぶ

    1. Visual Studio 17 2022 Win32
    2. Visual Studio 17 2022 x64
    3. Visual Studio 17 2022 arm64
    4. Visual Studio 16 2019 Win32
    5. Visual Studio 16 2019 x64
    6. Visual Studio 15 2017
    7. Visual Studio 14 2015
    8. Visual Studio 12 2013
    9. Visual Studio 11 2012
    a. Visual Studio 10 2010
    b. Visual Studio 9 2008
    select no

### cmakeを使用する場合

Visual Studio 2022 x86 の場合

    cmake -DCMAKE_GENERATOR="Visual Studio 17 2022" -DARCHITECTURE=win32 -P buildall.cmake

Visual Studio 2022 x64 の場合

    cmake -DCMAKE_GENERATOR="Visual Studio 17 2022" -DARCHITECTURE=x64 -P buildall.cmake`

Visual Studio 2022 arm64 の場合

    cmake -DCMAKE_GENERATOR="Visual Studio 17 2022" -DARCHITECTURE=arm64 -P buildall.cmake`

## MinGW 共通

各々の環境のcmakeを使用する

    cmake -DCMAKE_GENERATOR="Unix Makefiles" -DARCHITECTURE=i686 -P buildall.cmake

# 各フォルダについて

## 生成されるライブラリフォルダ

- 次のフォルダにライブラリの `*.h` , `*.lib` が生成される
    - `cJSON`
    - `oniguruma_{compiler}`
    - `libressl_{compiler}`
    - `SFMT_{compiler}`
    - `zlib_{compiler}`

## download アーカイブダウンロードフォルダ

- ダウンロードしたアーカイブファイルが置かれる
- 自動でダウンロードされる
- ダウンロードされていると再利用する
- ビルド後、参照する必要がなければ削除できる

## build ビルドフォルダ

- `build/oniguruma/{compiler}/` などの下でビルドされる
- 再ビルドするときは、あらかじめ削除すること
- ビルド後、参照する必要がなければ削除できる
