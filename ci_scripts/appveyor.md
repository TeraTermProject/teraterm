AppVeyorの使用
==============

## プロジェクト作成 / 設定

- Select repository for your new project
  - Generic の Git を選択
  - Clone URL
    - https://github.com/TeraTermProject/teraterm.git など
    - Authentication
      - None (public repository)
    - push [Add Git repository] button
- 設定
  - Settings/General
    - Project name 設定する
  - Default branch
    - main など
  - Custom configuration .yml file name (重要)
    - AppVeyorからアクセスできるところに appveyor_*.ymlを置く
    - https://raw.githubusercontent.com/TeraTermProject/teraterm/main/ci_scripts/appveyor_vs2022_bat.yml など
  - push [save] button

appveyor_vs*_bat.yml
====================

Windows image の Visual Studio を使用したビルド

## build

- Current build を選ぶ
- New Build を押す
  - libs/
    - 最初は libs/ のライブラリをビルドするのに時間がかかる
    - 2度目以降は lib/ 以下はキャッシュされる
    - 30分/jobぐらい
  - teraterm
- Artifacts にsnapshot.zip ができている


appveyor_release_bat.yml
========================

リリース用

- appveyor_vs2022_bat.yml がベース
- キャッシュを使用しない


appveyor_ubuntu2004.yml
=======================

Linux image(Ubuntu2004) の MinGW を利用したビルド


appveyor_mix.yml
================

いくつかのイメージを使って Visual Studio, Mingw を使って一気にビルド

最近使用していない

build_local_appveyor_*
======================

- ローカルで build_appveyor.bat をテストするための bat ファイル
- Visual Studio と msys2 を使用

ローカルでのテスト(Linux)
=======================

## ビルド準備/WSL

- ストアで debian をインストールする
  - 21-05-02時点で debian 10(buster)
- debian を起動
- 次のコマンドを実行

```
sudo apt-get update
sudo apt-get -y upgrade
sudo apt-get -y install cmake perl subversion
sudo apt-get -y install g++-mingw-w64
sudo apt-get -y install fp-utils
cd /path/to/teraterm
```

## ビルド準備/debian

- WSLと同じ
- 次のディストリビューション/バージョンに含まれているmingwでのビルドは確認した

```
$ cat /etc/os-release
PRETTY_NAME="Debian GNU/Linux bookworm/sid"
NAME="Debian GNU/Linux"
VERSION_CODENAME=bookworm
ID=debian
HOME_URL="https://www.debian.org/"
SUPPORT_URL="https://www.debian.org/support"
BUG_REPORT_URL="https://bugs.debian.org/"
```

## ビルド

gcc 32bit

```
cmake -DCMAKE_GENERATOR="Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake -P buildall.cmake
cmake -P ci_scripts/build_local_appveyor_mingw.cmake
```


gcc 64bit
- `cmake -DCOMPILER_64BIT=ON -P ci_scripts/build_local_appveyor_mingw.cmake`

msys64
- [build_local_appveyor_mingw_cmake.bat](build_local_appveyor_mingw_cmake.bat)参照
