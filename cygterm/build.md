# cygwin に関連するプログラムなど

- cyglaunch.exe
  - Cygwinのインストール先をPATHに追加してcygtermを実行するプログラム
- cygterm.exe
  - cygwinのシェルとTera Termの橋渡しをするプログラム
  - cygwin用64bit, 32bitをビルド可能
    - cygwinのとき、コンパイラパッケージをインストールしておく
- cygtool.dll (installer\cygtool)
  - インストーラから使用するdll


# cygterm, cyglaunchのビルド

cyglaunch は、実行に Cygwin を必要としないよう MinGW-w64 でコンパイルする

## 準備

- 32bit Cygwin
- 次のパッケージをインストール
  - gcc-core
  - mingw64-i686-gcc-core
  - make
  - tar
  - gzip

- 64bit Cygwin
- 次のパッケージをインストール
  - gcc-core
  - mingw64-x86_64-gcc-core
  - make


## ビルド

- cygterm フォルダで次のコマンドを実行
  - `make`

# リリース用バイナリに使われるコンパイラ
- cygterm.exe
  - Cygwin 32bit / gcc-core (gcc = i686-pc-cygwin-gcc)
- cygterm+-x86_64/cygterm.exe
  - Cygwin 64bit / gcc-core (gcc = x86_64-pc-cygwin-gcc)
  - Cygwin 64bit でビルドしたバイナリをリポジトリにコミットする
- cyglaunch.exe
  - Cygwin 32bit / mingw64-i686-gcc-core (i686-w64-mingw32-gcc)
- cygtool.dll
  - cl (Visual Studio)

