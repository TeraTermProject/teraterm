
# libフォルダ

- teratermが利用する外部のライブラリをビルドするためのフォルダ
- コンパイラ向けに各々ビルドする
- 1度ビルドしてライブラリを生成しておく

# ビルド手順

## Visual Studioの場合

- buildall_cmake.bat を実行
- cmakeを選択
- コンパイルする Visual Studioを選ぶ

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
