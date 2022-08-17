# release.bat について

- Tera Term をビルドするためのスクリプト(Windows用batファイル)
  - ミスなくリリース用バイナリを作成することを目的としている
  - 使用ライブラリをダウンロード、展開、ビルドできる
- 次の機能がある
  - ビルド環境の準備
    - 環境変数 PATH の設定
  - 使用するライブラリをダウンロード、展開、ビルド
  - Tera Term をビルド、アーカイブ、インストーラー作成
  - 使用ツールのパスやバージョンなどを表示
  - ビルドできる状態にセットアップして cmd.exe を起動する
- ビルドに使用するツールの指定について
  - 各ツールのデフォルトのインストールフォルダを探す
  - 個別に使用したいツールを明示する場合は toolinfo.bat で指定する
    - 参考 toolinfo_sample.bat
  - 使用ツールは次のドキュメントを参照
    - [doc/ja/html/reference/develop.html](../doc/ja/html/reference/develop.html)
- Tera Termをビルドしたときの最終的な生成ファイル
  - Release時
    - installer/Output/teraterm-x.y.exe
    - installer/Output/teraterm_x.y.zip
    - installer/Output/teraterm_x.y_pdb.zip
  - 通常ビルド(snapshot)時
    - installer/Output/teraterm-x.y-rREV-YYMMDD_HHMMSS-username-snapshot.exe
    - installer/Output/teraterm-x.y-rREV-YYMMDD_HHMMSS-username-snapshot.zip
    - installer/Output/teraterm-x.y-rREV-YYMMDD_HHMMSS-username-snapshot_pdb.zip

## pdbファイル

- pdb = program database files (symbol files)
- デバグ時に使用する

### 使い方

- 次のファイルを同じフォルダに配置する
  - クラッシュしたときのミニダンプ
    - teraterm_rREV_YYMMDD-HHMMSS-PID.dmp
  - exe,dll
  - pdb
- ミニダンプをダブルクリックしてVisual Studioを起動する
- アクションの[ネイティブのみでデバグ]をクリック
