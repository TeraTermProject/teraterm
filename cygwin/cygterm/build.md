# cygtermのビルドについて

- 64bit Cygwinでビルドできるのは 64bit Cygwin用バイナリのみ

- Cygwin 3.4.0-1 (2022-12-04リリース) から32bit Windows サポートがなくなった
  - 以前は64bit Cygwinで32bit Cygwin用バイナリをビルドすることができた
  - 32bit Cygwinで64bit Cygwin用バイナリをビルドすることもできた

## 準備

Cygwin環境でビルドする場合、次のパッケージをインストール

- gcc-core
- gcc-g++
- make
- tar
- gzip
- setupを使ったインストール例
    - `setup-x86_64.exe --quiet-mode --packages gcc-core --packages gcc-g++ --packages make`

## ビルド

- 次のファイルを実行
  - `build.bat`

## cygwinのパッケージについて

- パッケージに含まれるファイルは次のURLから調べることができる
  - https://cygwin.com/packages/

# msys2termのビルドについて

MSYS2 のシェルを起動する。
Git BashなどをTera Term 内で動作させることができる。

## 準備

- MSYS2 MSYSで行う
- makeコマンドの時 `pacman -S gcc make`
- cmakeビルドの時 `pacman -S gcc cmake`

## ビルド

- make
  - MSYS2 MSYS からシェルを起動
  - `make` 実行
- cmakeビルドのとき
  - teratermをビルドすると自動的に
    バイナリフォルダ(ttermpro.exeと同じフォルダ)に作成される

# build_cygterm.cmake を使ったビルド

cmd.exe で次のコマンドを実行、`build_all/cygterm/`にバイナリが生成される

```
..\..\buildtools\install_cygwin.bat
..\..\buildtools\install_msys2.bat
mkdir build_all
cd build_all
mkdir cygterm
cmake -DCMAKE_INSTALL_PREFIX=%CD%\cygterm -P ../build_cygterm.cmake
```
