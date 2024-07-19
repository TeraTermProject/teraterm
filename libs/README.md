# ライブラリバージョンアップ時の変更箇所

## 変更をコミットする箇所

- doc/(en|ja)/html/about/history.html
  - 関係するコンポーネントの (その他|Misc) セクションに追記する
- doc/(en|ja)/html/reference/develop.html
  - 使用するライブラリのバージョンを変更する
- libs/download.cmake（リリース用スクリプト用）
  - SRC_URL のバージョンを変更する
  - DIR_IN_ARC のバージョンを変更する
  - ARC_HASH を変更する。（ダウンロード破損チェック用）
    - 公式配布元のハッシュ値を使用する。（LibreSSL, Oniguruma, zlib）
    - 公式がハッシュ値を公開していないものは、手元で求める。（argon2, cJSON, SFMT）
  - CHECK_HASH を変更する。（バージョンチェック用）
    - バージョンチェックに使うファイルのハッシュ値を手元で求める。
- libs/xxx.cmake（cmake ビルド用）
  - SRC_ARC_HASH_SHA256 を変更する

## ファイルのハッシュ値の求めかた

```
% sha256sum foo.bar
% openssl dgst -sha256 foo.bar
```
