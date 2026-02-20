# 各種バッチファイル/スクリプトの使い方

## ビルドの全体像

```
msbuild.yml
  |
  +--> release.bat
  |      +--> build_common.bat
  |             +--> doc/makechm.bat
  |             +--> makelang.bat
  |
  +--> release.bat アーキテクチャごとに呼び出し
  |      +--> libs/buildall.bat
  |      |      +--> libs/buildoniguruma6.bat
  |      |      +--> libs/buildzlib.bat
  |      |      +--> libs/buildlibressl.bat
  |      |      +--> libs/buildSFMT.bat
  |      +--> build_arch.bat
  |      +--> collect_files.bat
  |      +--> create_packages.bat
  +--> iscc_signed.cmake アーキテクチャごとに呼び出し

release.bat を直接起動したり、コマンドプロンプトから release.bat を呼び出すこともできます
```

## 解説

- msbuild.yml
  - GitHub Actions workflow ファイル

- check_sjis_code.bat
  - 英語版HTMLヘルプ(..\doc\en\)に日本語(Shift_JIS)が含まれていないかを調べ、結果を"result.txt"に書き出す。

- release.bat
  - ビルド(リリース)用batファイル
  - [release.md](release.md) 参照してください。

- build_common.bat
  - ヘルプを作成します
  - cygterm.exe をビルドします
  - msys2term.exe をビルドします
  - cygterm.+.tar.gz を作成します
  - makelang.bat を呼び出します
  - 生成物を一か所に集めます
    - ./teraterm_common/

- makelang.bat
  - lng ファイルを作成します

- libs/buildall.bat
  - ライブラリをビルドします。

- build_arch.bat
  - Visual Studio プロジェクトをビルドします

- collect_files.bat
  - バイナリを一か所に集めます
    - ./teraterm-%arch%/
    - ./teraterm-%arch%_pdb/

- create_packages.bat
  - インストーラー、zipを作成します。

- teraterm.iss
  - インストーラー(Inno Setup)スクリプトファイル。

- iscc_signed.cmake
  - msbuild.yml から呼び出され、署名済みバイナリを含んだインストーラを作成します

- codesigning.bat
  - 実行ファイルにコードサイニング証明書を付与します。（古いテスト版で不使用）


## 環境変数

workflow では build_arch ジョブの arch で指定される。TARGET は arch を元に自動設定される。
コマンドプロンプトから指定するときは、環境変数に arch と TARGET を set する。

- VS_VERSION
  - インストールされている Visual Studio を探すために使用される
  - 設定されていない場合は 2022 が使用される
- arch
  - インストーラ・ポータブルのファイル名・フォルダ名に使用される
- TARGET
  - Visual Studio プロジェクト
  - devenv の SolnConfigName の指定に使用される
    - Visual Studio での $(Platform) になる
    - ファイルの出力先フォルダ名に使用される
  - ライブラリ
    - cmake の -A に使用される
    - .lib ファイルの出力先フォルダ名に使用される
- ARCHITECTURE
  - TARGET から自動設定され、VsDevCmd.bat の -arch に使用される
  - cl の選択に使用され、cl はライブラリのコンパイルに使用される
- HOST_ARCHITECTURE
  - TARGET から自動設定され、VsDevCmd.bat の -host_arch に使用される
  - cl の選択に使用され、cl はライブラリのコンパイルに使用される


## フォルダに出力されるファイル
- 通常時
  - OUTPUT=teraterm-${VERSION}-%arch%-${DATE_TIME}-${VCSVERSION}-snapshot
  - 例: teraterm-x.y.z-x86-dev-YYMMDD_HHMMSS-githash-snapshot
- リリース時
  - OUTPUT=teraterm-${VERSION}-%arch%
  - 例: teraterm-x.y.z-x64

```
release/Output/
  build/
    teraterm-common/      ... build_common.bat
    teraterm-%arch%/      ... build_arch.bat
    teraterm-%arch%_pdb/  ... build_arch.bat
  ${OUTPUT}/          ... collect_files.bat
                          Output/build/teraterm/ をコピー
  ${OUTPUT}_pdb/      ... collect_files.bat
                          Output/build/teraterm_pdb/ をコピー
  ${OUTPUT}.exe       ... collect_files.bat
                          Output/build/teraterm/ からファイルを拾う
  ${OUTPUT}.zip       ... collect_files.bat
                          ${OUTPUT}/ を圧縮
  ${OUTPUT}_pdb.zip   ... collect_files.bat
                          ${OUTPUT}_pdb/ を圧縮
  ${OUTPUT}.sha256sum ... collect_files.bat
  ${OUTPUT}.sha512sum ... collect_files.bat
  portable/
    teraterm-%arch%/ ... create_package.bat
                         Output/build/teraterm-%arch%/ をコピー
  portable_signed/
    teraterm-%arch%/ ... msbuild.yml
                         署名して SignPath からダウンロード
                         最終的に ${OUTPUT}/ にリネームされる
    ${OUTPUT}/       ... msbuild.yml
                         Output/portable_signed/teraterm-%arch%/ をリネーム
  setup/
    ${OUTPUT}.exe ... iscc_signed.cmake
                      Output/portable_signed/teraterm-%arch%/ からファイルを拾う
  setup_signed/
    ${OUTPUT}.exe ... msbuild.yml
                      署名して SignPath からダウンロード
  release/              ... msbuild.yml
    ${OUTPUT}.exe       ... Output/setup_signed/${OUTPUT}.exe をコピー
    ${OUTPUT}.zip       ... Output/portable_signed/${OUTPUT}/ を圧縮
    ${OUTPUT}_pdb.zip   ... Output/${OUTPUT}_pdb.zip をコピー
    ${OUTPUT}.sha256sum
    ${OUTPUT}.sha512sum
```


## artifact に出力されるファイル
- buildinfo
- teraterm_common
- BUILD-${{ github.run_number }}-${{ github.run_id }}-VS${{ matrix.VS_VERSION }}-${{ matrix.arch }}-zip
- BUILD-${{ github.run_number }}-${{ github.run_id }}-VS${{ matrix.VS_VERSION }}-${{ matrix.arch }}-pdb
- BUILD-${{ github.run_number }}-${{ github.run_id }}-VS${{ matrix.VS_VERSION }}-${{ matrix.arch }}-installer
- BUILD-${{ github.run_number }}-${{ github.run_id }}-VS${{ matrix.VS_VERSION }}-${{ matrix.arch }}-sha256sum
- BUILD-${{ github.run_number }}-${{ github.run_id }}-VS${{ matrix.VS_VERSION }}-${{ matrix.arch }}-sha512sum
- portable_unsigned-${{ matrix.arch }} ... 署名のために SignPath に渡す用
- setup_unsigned-${{ matrix.arch }}- - ... 署名のために SignPath に渡す用
- ${OUTPUT}_ZIP
- ${OUTPUT}_EXE
- ${OUTPUT}_FILES
- checksum ... リリース用ファイルすべての checksum


