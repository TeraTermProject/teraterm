# svnrev

- ソースツリーのgit(又は svn)の情報をヘッダファイル等に書き出すためのツール
  - ソース用のヘッダフィル
    - 例 `../../teraterm/common/svnversion.h`
  - CI用のbatファイル
    - 例 `sourcetree_info.bat`
  - cmake用ファイル
    - 例 buildフォルダの `build_config.cmake`
  - json用ファイル
    - 例 buildフォルダの `build_config.json`
  - Inno Setup用ファイル
    - 例 installフォルダの `../../installer/build_config.isl`

## 準備

- perl を実行できるようにしておく
  - perl が実行できるよう環境変数 PATH を設定する
  - または、../buildtools/libs/perl があれば使用する
    - ../buildtools/getperl.bat をダブルクリックすると
      strawberry perl が buildtools/perl に展開される
  - または、いくつかのメジャーなperlを探して見つければ使用する
  - perl が見つからない場合は svnversion.default.h が使用される
- git(又は svn)をインストールしておく
  - Windows用gitの例
    - git
      - https://git-scm.com/
  - Windows用svnの例
    - Subversion for Windows
      - http://sourceforge.net/projects/win32svn/
    - TortoiseSVN の command line client tools
      - https://tortoisesvn.net/
  - 標準的なインストールフォルダから自動的に実行ファイルを探す
  - 見つからない場合は環境変数 PATH にあるプログラムを実行する
  - toolinfo.txt にツールのパスを書いておくと優先して使用される
  - ツールが実行できない場合もヘッダファイルは作成される

# ヘッダの作成方法

- Visual Studioを利用している場合
  - 自動的に生成される
  - ttermpro プロジェクトのビルド前のイベントでsvnrev.batが呼び出される
- Windows の場合
  - svnrev.bat をダブルクリックでヘッダが作成される
- cmake の場合
  - ビルド用のファイルをジェネレート時に自動で生成される
  - build フォルダの build_config.cmake を削除してビルドすると再生成される
- コマンドラインで直接実行する場合
  - `perl svnrev.pl --root ../.. --header ../../teraterm/common/svnversion.h`

## svnrev.pl のオプション

- --architecture
  - Win32 or x86 or arm64
  - default Win32
- --root
  - プロジェクトファイルのパス
  - '.svn/' 又は '.git/' の存在するパス
- --header
  - ヘッダファイルのパス
- --cmake
  - cmakeファイルのパス
- --json
  - jsonファイルのパス
- --svn
  - svnコマンドのパス
- --git
  - gitコマンドのパス
- --isl
  - Inno Setup用ファイルのパス

オプションが toolinfo.txt より優先される

## 使用例

このフォルダで
```
$ perl svnrev.pl --architecture x64 --cmake test.cmake --json test.json --isl test.isl --verbose
root=../..
tt_version_major=5
tt_version_minor=6
tt_version_patch=0
tt_version_substr=dev
svn="C:/Program Files/TortoiseSVN/bin/svn.exe"
git="C:/Program Files/Git/bin/git.exe"
header="svnversion.h"
bat="sourcetree_info.bat"
cmake="test.cmake"
json="test.json"
overwrite 0
architecture="x64"
SVNREVISION 1234567
RELEASE 0
BRANCH_NAME main
update 'svnversion.h'
update 'sourcetree_info.bat'
update 'test.cmake'
update 'test.json'
update 'test.isl'
```
