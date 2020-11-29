# svnrev

- svn(又は git)の情報からヘッダファイルとバッチファイルを作成する
  - `../teraterm/ttpdlg/svnversion.h`
  - `sourcetree_info.bat`
  - buildフォルダの `build_config.cmake` (cmake時のみ)

## 準備

- perl を実行できるようにしておく
  - perl が実行できるよう環境変数 PATH を設定する
  - または、../buildtools/libs/perl があれば使用する
    - ../buildtools/getperl.bat をダブルクリックすると
      strawberry perl が buildtools/perl に展開される
  - または、いくつかのメジャーなperlを探して見つければ使用する
  - perl が見つからない場合は svnversion.default.h が使用される
- svn(又は git)を実行できるようにする
  - svn が実行できるよう環境変数 PATH を設定する
  - または、toolinfo.txt にツールのパスを書いておく
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
