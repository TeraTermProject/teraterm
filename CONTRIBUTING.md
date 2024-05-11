# CONTRIBUTING

開発を手伝っていただければとても助かります。

- ドキュメント、lngファイルの不備の指摘、修正、英訳(翻訳)など
- 動作の不具合の指摘、修正
- 不具合などのレポート(Issue)の対応
  - 他の困っている方をサポートする、不具合の再現チェックなど

次のドキュメントを参考にしてください。
- [連絡先,各種情報へリンク](https://teratermproject.github.io/manual/5/ja/about/contacts.html)

## プログラム、ドキュメントの修正

GitHubを使用しています。
修正を送る(Pull Request(PR)を送る)手順は次のようになります。
参考にしてください。

- GitHub上でTera TermリポジトリをForkして自分のリモートリポジトリを作成する
- 自分のリモートリポジトリをローカルにcloneする
- プログラムの修正の場合、Tera Termのビルド
  - [ビルド手順](https://teratermproject.github.io/manual/5/ja/reference/develop-build.html#build-quick) を参照ください
- ブランチを作成して修正
- ローカルの修正を自分のリモートリポジトリにpush
  - 修正したソースツリーがGitHubに置いてあれば、[AppVeyorでビルド](https://raw.githubusercontent.com/TeraTermProject/teraterm/main/ci_scripts/appveyor.md)することもできます
- Pull Requestを送る

Issueで問題を指摘して、解決のためのPRをいただいてもいいですし、
PRを直接いただいてもokです。

修正にドキュメント(マニュアル(日/英)、修正履歴)が含まれていると
main へのマージがスムーズに進みやすいです。

修正はプルリクエストのレビューを使って
内容の確認や追加の修正をお願いすることがあります。

GitHub よりもカジュアルに使える Discord があります。
[wiki](https://github.com/TeraTermProject/teraterm/wiki/Discord)を参照ください。
