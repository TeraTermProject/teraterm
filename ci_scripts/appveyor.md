AppVeyorの使用
==============

現在のところCIのために利用します。

## プロジェクト作成 / 設定

- Select repository for your new project
  - Subversion を選択
  - Checkout URL
    - http://svn.osdn.net/svnroot/ttssh2/trunk
    - Authentication None
  - プロジェクトができる
- 設定
  - Settings/General
    - Project name 設定する
  - Custom configuration .yml file name (重要)
    - AppVeyorからアクセスできるところに appveyor_*.ymlを置く
    - https://osdn.net/projects/ttssh2/scm/svn/blobs/head/trunk/ci_scripts/appveyor_vs2022.yml?export=raw など


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

appveyor_ubuntu2004.yml
=======================

Linux image(Ubuntu2004) の MinGW を利用したビルド


appveyor_mix.yml
================

いくつかのイメージを使って Visual Studio, Mingw を使って一気にビルド

最近使用していない

## build_local_appveyor_*

- ローカルで build_appveyor.bat をテストするための bat ファイル
- Visual Studio と msys2 を使用
