# 各種バッチファイル/スクリプトの使い方

- check_sjis_code.bat
  - 英語版HTMLヘルプ(..\doc\en\)に日本語(Shift_JIS)が含まれていないかを調べ、結果を"result.txt"に書き出す。

- realease.bat
  - ビルド(リリース)用batファイル
  - [release.md](release.md) 参照してください。
  - 次のファイルを呼び出します。
    - makearchive.bat
    - iscc.bat

- makearchive.bat
  - Visual Studioコマンドプロンプトから実行すると、
    - ヘルプを作成
    - ソースコードをビルド、バイナリを作成
    - バイナリを一か所に集める
      - ./teraterm/
      - ./teraterm_pdb/
  - 次のファイルを呼び出します。
    - makechm.bat
    - build.bat

- iscc.bat
  - インストーラー,zipを作成します。

- build.bat
  - バイナリをビルドします。
  - 次のファイルを呼び出します。
    - makelang.bat

- makelang.bat
  - lng ファイルを作成します

- teraterm.iss
- teraterm_cmake.iss
  - インストーラー(Inno Setup)スクリプトファイル。
  - teraterm_cmake.iss は cmakeビルド時に使用します。

- codesigning.bat
  - 実行ファイルにコードサイニング証明書を付与します。
