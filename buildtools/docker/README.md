# build with MinGW + docker (experimental)

- Thread model win32版のMinGWを使ってビルド
- dockerでビルド環境を作成、docker中でビルド

- linuxビルドの制限
  - cygterm, msys2termはビルドできない
  - インストーラは作成できない

## 準備

- Windowsの場合
  - Docker Desktop を使えるようにする
- Linuxの場合
  - Dockerを使えるようにする

## ビルド例

- `docker_build.bat` (windowsのとき) / `docker_build.sh` (linuxのとき) を実行
  - docker内のbashが起動する
- `bash ./build.sh` を実行
- このフォルダに teraterm*.zip ができる
