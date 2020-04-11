# svnrev

- svn(又は git)の情報からヘッダファイルとバッチファイルを作成する
  - `../teraterm/ttpdlg/svnversion.h`
  - `svnversion.bat`

## 準備

- svn(又は git)を実行できるようパスを設定する
  - 実行できない場合もヘッダファイルは作成されます
- perlを実行できるようパスを設定する
  - libs/perl があれば利用します
    - libs/getperl.bat をダブルクリックすると
      strawberry perl を libs/perl に展開します

# ヘッダの作成方法

- Visual Studioを利用している場合
  - svnrev_perl プロジェクトから自動的に生成される
- Windows の場合
  - svnrev.bat をダブルクリックでヘッダが作成される
- cmake の場合
  - buildフォルダで次のように実行する
  - `cmake --build . --target svnversion_h`
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
