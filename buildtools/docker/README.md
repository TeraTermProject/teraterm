# build with MinGW + docker (experimental)

- Thread model win32版のMinGWを使ってビルド
- dockerでビルド環境(linux)を作成、docker中でビルド

- linuxビルドの制限
  - cygterm, msys2termはビルドできない
  - インストーラは作成できない

## 準備 Windowsの場合

- Docker Desktop を使えるようにする
- 次のファイルを取得、次のコマンドを実行
  - `docker_build.bat`
  - `Dockerfile`
  - `build.sh`
- `docker_build.bat`を実行
- dockerのコンテナ内のbashが起動する

### 実行例

```
curl -O https://raw.githubusercontent.com/TeraTermProject/teraterm/refs/heads/main/buildtools/docker/docker_build.bat
curl -O https://raw.githubusercontent.com/TeraTermProject/teraterm/refs/heads/main/buildtools/docker/Dockerfile
curl -O https://raw.githubusercontent.com/TeraTermProject/teraterm/refs/heads/main/buildtools/docker/build.sh
docker_build.bat
```
## 準備 Linux(WSL)の場合

- Dockerを使えるようにする
- 次のファイルを取得、次のコマンドを実行
  - `docker_build.sh`
  - `Dockerfile`
  - `build.sh`
- `docker_build.bat`を実行
- dockerのコンテナ内のbashが起動する


## ビルド

- `bash ./build.sh` を実行
- dockerを起動したフォルダに teraterm*.zip ができる

## docker-compose.yml

`docker compose run --rm dev bash`
作成中
