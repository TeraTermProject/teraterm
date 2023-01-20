# cygtermのビルドについて

- リリース時には 64bit バイナリと 32bit バイナリの両方をビルドしたい
- 64bit Cygwinを使いたい
  - 近い将来 32bit Cygwinがなくなる

## 準備

64bit Cygwin では 32bit Cygwin クロスコンパイラを利用できるようにする
32bit Cygwin では 64bit Cygwin クロスコンパイラを利用できるようにする
リリースでは 64bit Cygwin を使用する

- 64bit Cygwin時(将来はこれだけになる)
- 次のパッケージをインストール
  - gcc-core
  - gcc-g++
  - make
  - cygwin32-gcc-core (32bit Cygwin 用バイナリを出力するクロスコンパイラ)
  - cygwin32-gcc-g++ (同上)
  - tar
  - gzip
  - setupを使ったインストール例
    - `setup-x86_64.exe --quiet-mode --packages cygwin32-gcc-g++ --packages cygwin32-gcc-core`

- 32bit Cygwin時(近い将来なくなる)
- 次のパッケージをインストール
  - gcc-core
  - gcc-g++
  - make
  - cygwin64-gcc-core (64bit Cygwin 用バイナリを出力するクロスコンパイラ)
  - cygwin64-gcc-g++ (同上)
  - tar
  - gzip
  - setupを使ったインストール例
    - `setup-x86.exe --quiet-mode --packages cygwin64-gcc-g++ --packages cygwin64-gcc-core`

## ビルド

- 次のファイルを実行
  - `build.bat`

## cygwinのパッケージについて

- パッケージに含まれるファイルは次のURLから調べることができる
  - https://cygwin.com/packages/
