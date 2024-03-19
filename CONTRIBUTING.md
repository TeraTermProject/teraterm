# CONTRIBUTING

開発を手伝っていただければとても助かります。

- 不具合などのレポート、レポートのチェック
  - [連絡先,各種情報へリンク](https://teratermproject.github.io/manual/5/ja/about/contacts.html)
- ドキュメントの修正、英訳など
- 不具合の修正

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

