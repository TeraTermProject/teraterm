# cygwin に関連するプログラムなど

- cyglaunch
  - cygterm(または msys2term) を実行するプログラム
- cygterm
  - cygwin(msys2)のシェルとTera Termの橋渡しをするプログラム
  - cygwin用64bit, 32bit, msys2用(msys2term) をビルド可能
    - cygwinのとき、コンパイラパッケージをインストールしておく
      - cygwin 64bit環境時は cygwin32-gcc-core と cygwin32-gcc-g++
      - cygwin 32bit環境時は cygwin64-gcc-core と cygwin64-gcc-g++
- cygterm_build
  - cmakeビルド用 cygterm(とmsys2term) をビルドするためのフォルダ
- cygtool
  - インストーラから使用するdll
- cygtool_build
  - cmakeビルド用 cygtool をビルドするためのフォルダ
- cyglib
  - ttermpro, cyglaunch, cygtool から使用するライブラリ
