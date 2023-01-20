# cygwin に関連するプログラムなど

- cyglaunch
  - Cygwinのインストール先をPATHに追加してcygterm(または msys2term) を実行するプログラム
  - Visual Studio でビルドする cf. cyglaunch/README.md
- cygterm
  - cygwin(msys2)のシェルとTera Termの橋渡しをするプログラム
  - cygwin用64bit, 32bit, msys2用(msys2term) をビルド可能
    - cygwinのとき、コンパイラパッケージをインストールしておく cf. cygterm/build.md
- cygterm_build
  - cmakeビルド用 cygterm(とmsys2term) をビルドするためのフォルダ
- cygtool
  - インストーラから使用するdll
- cygtool_build
  - cmakeビルド用 cygtool をビルドするためのフォルダ
- cyglib
  - ttermpro, cyglaunch, cygtool から使用するライブラリ


# リリース用バイナリに使われるコンパイラ
- cygterm+-i686/cygterm.exe
  - Cygwin 64bit / gcc-g++ (x86_64-pc-cygwin-gcc)
- cygterm+-x86_64/cygterm.exe
  - Cygwin 64bit / mingw32-gcc-g++ (i686-pc-cygwin-gcc)
- cyglaunch.exe
  - cl (Visual Studio)
- cygtool.dll
  - cl (Visual Studio)

