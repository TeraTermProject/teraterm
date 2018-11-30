
# libフォルダ

- teratermが利用する外部のライブラリをビルドするためのフォルダ
- コンパイラ向けに各々ビルドする
- 1度ビルドしてライブラリを生成しておく

# ビルド手順

## Visual Studioの場合

### batファイルを使用する場合

- buildall_cmake.bat を実行
- cmakeを選択
- コンパイルに使用する Visual Studioを選ぶ

### cmakeを使用する場合

- cmakeを使える状態にしてcmakeを実行
	- `cmake -DCMAKE_GENERATOR="Visual Studio 15 2017" -P buildall.cmake`
	- `cmake -DCMAKE_GENERATOR="Visual Studio 15 2017 Win64" -P buildall.cmake`
- Visual Studio 2005の場合は、cmakeのバージョン3.11.4以前を使用する
	- `cmake -DCMAKE_GENERATOR="Visual Studio 8 2005" -P buildall.cmake`

## MinGW 共通

- Cygwin,MSYS2,linux(wsl)上のMinGWでビルドできる
- 各環境で動作するcmake,make,(MinGW)gcc,perlが必要
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

- ダウンロードしたアーカイブファイルが置かれます
- 自動でダウンロードされます
- ダウンロードされていると再利用する

## build ビルドフォルダ

- build/oniguruma_{compiler}/ などの下でビルドされます
- ビルド後は参照する必要がなければ削除できます。
