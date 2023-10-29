AppVeyorの使用
==============

git(GitHubも含む)リポジトリからのビルド

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

## build_local_appveyor_*

- ローカルで build_appveyor.bat をテストするための bat ファイル
- Visual Studio と msys2 を使用
