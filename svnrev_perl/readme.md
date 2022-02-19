# svnrev

- ソースツリーのsvn(又は git)の情報をヘッダファイル等に書き出すためのツール
  - ソース用のヘッダフィル
    - 例 `../teraterm/ttpdlg/svnversion.h`
  - CI用のbatファイル
    - 例 `sourcetree_info.bat`
  - cmake用ファイル
    - 例 buildフォルダの `build_config.cmake`

## 準備

- perl を実行できるようにしておく
  - perl が実行できるよう環境変数 PATH を設定する
  - または、../buildtools/libs/perl があれば使用する
    - ../buildtools/getperl.bat をダブルクリックすると
      strawberry perl が buildtools/perl に展開される
  - または、いくつかのメジャーなperlを探して見つければ使用する
  - perl が見つからない場合は svnversion.default.h が使用される
- svn(又は git)をインストールしておく
  - Windows用svnの例
    - Subversion for Windows
      - http://sourceforge.net/projects/win32svn/
    - TortoiseSVN の command line client tools
      - https://tortoisesvn.net/
  - Windows用gitの例
    - git
      - https://git-scm.com/
  - 標準的なインストールフォルダから自動的に実行ファイルを探す
  - 見つからない場合は環境変数 PATH にあるプログラムを実行する
  - toolinfo.txt にツールのパスを書いておくと優先して使用される
  - ツールが実行できない場合もヘッダファイルは作成される

# ヘッダの作成方法

- Visual Studioを利用している場合
  - svnrev_perl プロジェクトから自動的に生成される
- Windows の場合
  - svnrev.bat をダブルクリックでヘッダが作成される
- cmake の場合
  - ビルド用のファイルをジェネレート時に自動で生成される
  - build フォルダの build_config.cmake を削除してビルドすると再生成される
- コマンドラインで直接実行する場合
  - `perl svnrev.pl --root .. --header ../teraterm/ttpdlg/svnversion.h`

## svnrev.pl のオプション

- --root
  - プロジェクトファイルのパス
  - '.svn/' 又は '.git/' の存在するパス
- --header
  - ヘッダファイルのパス
- --svn
  - svnコマンドのパス
- --git
  - gitコマンドのパス

オプションが toolinfo.txt より優先される
