
# libsフォルダ

- Tera Term のビルドに利用する外部のライブラリを置いておくためのフォルダ
- 各コンパイラ向けにソース/ライブラリ/実行ファイルを置いておく
- ライブラリはあらかじめ1度だけ生成しておく

# 準備

## Visual Studio

- cmake
	- PATHが通してあればok
	- cygwinのcmakeはつかえない(Visual Studioをサポートしていない)
	- Visual Studio 2005 を使う場合は cmake 3.11.4 を使用する必要がある
- perl
	- OpenSSL のコンパイル、ドキュメントファイルの文字コード・改行コード変換に必要
	- ActivePerl 5.8 以上、または cygwin perl
	- PATHが通っていなければ自動で探す

## MinGW 共通 (experimental)

- Cygwin,MSYS2,linux(wsl)上のMinGWでビルド可能
- 各環境で動作するcmake,make,(MinGW)gcc,(clang),perlが必要

# ビルド手順

必要なアーカイブを自動的にダウンロードするので、
インターネットが利用できる環境でビルドする必要がある

## Visual Studioの場合

### batファイルを使用する場合

buildall_cmake.bat を実行して使用する Visual Studioを選ぶ

    1. Visual Studio 16 2019
    2. Visual Studio 15 2017
    3. Visual Studio 14 2015
    4. Visual Studio 12 2013
    5. Visual Studio 11 2012
    6. Visual Studio 10 2010
    7. Visual Studio 9 2008
    8. Visual Studio 8 2005
    select no

VS2005を選択した場合、
このバッチファイルから cmake 3.11.4 をダウンロードして `libs\cmake-3.11.4-win32-x86` に
インストールできます。

### cmakeを使用する場合

Visual Studio 2019 x86 の場合

    cmake -DCMAKE_GENERATOR="Visual Studio 16 2019" -DARCHITECTURE=Win32 -P buildall.cmake

Visual Studio 2017 x86 の場合

    cmake -DCMAKE_GENERATOR="Visual Studio 15 2017" -P buildall.cmake

Visual Studio 2017 x64 の場合

    cmake -DCMAKE_GENERATOR="Visual Studio 15 2017 Win64" -P buildall.cmake`

Visual Studio 2005の場合は、cmakeのバージョン3.11.4以前を使用
(cmake が libs\cmake-3.11.4-win32-x86 にインストールしてある場合)

    libs\cmake-3.11.4-win32-x86\bin\cmake.exe" -DCMAKE_GENERATOR="Visual Studio 8 2005" -P buildall.cmake

## MinGW 共通

各々の環境のcmakeを使用する

    cmake -DCMAKE_GENERATOR="Unix Makefiles" -P buildall.cmake

# 各フォルダについて

## 生成されるライブラリフォルダ

- 次のフォルダにライブラリの `*.h` , `*.lib` が生成される
	- `oniguruma_{compiler}`
	- `openssl_{compiler}`
	- `putty`
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
