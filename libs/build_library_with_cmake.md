
# libフォルダ

- teratermが利用する外部のライブラリをビルドするためのフォルダ
- コンパイラ向けに各々ビルド
- 1度ビルドしてライブラリを生成しておく

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
- 各環境で動作するcmake,make,(MinGW)gcc,perlが必要

# ビルド手順

必要なアーカイブを自動的にダウンロードするので、
インターネットが利用できる環境でビルドする必要がある

## Visual Studioの場合

### batファイルを使用する場合

- 自動的にビルド
- buildall_cmake.bat を実行
- コンパイルに使用する Visual Studioを選ぶ
- VS2005の場合はcmake 3.11.4 をダウンロードして
  libs\cmake-3.11.4-win32-x86 にインストールする

### cmakeを使用する場合

- cmakeを使える状態にしてcmakeを実行
	- `cmake -DCMAKE_GENERATOR="Visual Studio 15 2017" -P buildall.cmake`
	- `cmake -DCMAKE_GENERATOR="Visual Studio 15 2017 Win64" -P buildall.cmake`
- Visual Studio 2005の場合は、cmakeのバージョン3.11.4以前を使用する
	- cmake が libs\cmake-3.11.4-win32-x86 にインストールしてある場合  
	  `"libs\cmake-3.11.4-win32-x86\bin\cmake.exe" -DCMAKE_GENERATOR="Visual Studio 8 2005" -P buildall.cmake`

## MinGW 共通

- 各々の環境のcmakeを使って
  `cmake -DCMAKE_GENERATOR="Unix Makefiles" -P buildall.cmake` を実行

# 各フォルダについて

## 生成されるライブラリフォルダ

- 次のフォルダにライブラリの`*.h`,`*.lib`が生成される
	- oniguruma_{compiler}
	- openssl_{compiler}
	- putty
	- SFMT_{compiler}
	- zlib_{compiler}

## download アーカイブダウンロードフォルダ

- ダウンロードしたアーカイブファイルが置かれる
- 自動でダウンロードされる
- ダウンロードされていると再利用する
- ビルド後、参照する必要がなければ削除できる

## build ビルドフォルダ

- build/oniguruma_{compiler}/ などの下でビルドされる
- ビルド後、参照する必要がなければ削除できる
