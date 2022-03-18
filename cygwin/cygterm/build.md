# cygtermのビルドについて

- リリース時に64bitと32bit両方をビルドしたい
- 64bit Cygwinを使いたい
  - 近い将来 32bit Cygwinがなくなる

## 準備

64bit Cygwinの32bit Cygwinクロスコンパイルを利用する
(32bit Cygwinの64bit Cygwinクロスコンパイルも利用できる)

- 64bit Cygwin時(将来はこれだけになる)
- 次のパッケージをインストール
  - gcc-core
  - gcc-g++
  - make
  - cygwin32-gcc-core
  - cygwin32-gcc-g++
  - setupを使ったインストール例
    - `setup-x86_64.exe --quiet-mode --packages cygwin32-gcc-g++ --packages cygwin32-gcc-core`

- 32bit Cygwin時(近い将来なくなる)
- 次のパッケージをインストール
  - gcc-core
  - gcc-g++
  - make
  - cygwin64-gcc-core
  - cygwin64-gcc-g++
  - setupを使ったインストール例
    - `setup-x86.exe --quiet-mode --packages cygwin64-gcc-g++ --packages cygwin64-gcc-core`

## ビルド

- 次のファイルを実行
  - `build.bat`

## cygwinのパッケージについて

- パッケージに含まれるファイルは次のURLから調べることができる
  - https://cygwin.com/packages/
