# release.bat について

## ビルドの全体像

[readme.md](readme.md) を参照してください。

## 解説

- Tera Term をビルドするためのスクリプト(Windows用batファイル)
  - ミスなくリリース用バイナリを作成することを目的としている
  - 使用ライブラリをダウンロード、展開、ビルドできる
- 実行前に環境変数を設定する
  - [readme.md](readme.md) を参照してください。
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
  - [readme.md](readme.md) を参照してください。

## pdbファイル

pdbファイルについては[dump](../doc/ja/html/reference/develop-memo.html#dump)を参照ください
