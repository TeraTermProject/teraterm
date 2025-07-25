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
  - バイナリから署名のないポータブルzip,インストーラを作成
  - コピー(ポータブル署名用)
    - src
      - ./teraterm/
    - dest
      - ./Output/portable/teraterm/
  - インストーラー(署名なし)
    - src
      - ./teraterm/
    - dest
      - ./teraterm-X.Y.Z.exe
          or
      - ./teratern-X.Y.Z-YYMMDD_HHMMSS-HASH-snapshot.exe
  - ポータブル(署名なし)
    - src
      - ./teraterm/
      - ./teraterm_pdb/
    - src(copy)
      - ./teraterm-X.Y.Z/
      - ./teraterm-X.Y.Z_pdb/
          or
      - ./teratern-X.Y.Z-YYMMDD_HHMMSS-HASH-snapshot/
      - ./teratern-X.Y.Z-YYMMDD_HHMMSS-HASH-snapshot_pdb/
    - dest
      - ./teraterm-X.Y.Z.zip
      - ./teraterm-X.Y.Z_pdb.zip
          or
      - ./teratern-X.Y.Z-YYMMDD_HHMMSS-HASH-snapshot.zip
      - ./teratern-X.Y.Z-YYMMDD_HHMMSS-HASH-snapshot_pdb.zip
  - hash(sha256,sha512)

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
