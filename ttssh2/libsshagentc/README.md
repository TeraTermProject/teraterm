# libsshagentc

ssh-agent と通信するためのライブラリ。
Tera Term 以外でも使えるよう考慮。

ファイル/フォルダ

- libputty.h
  - libsshagentc インターフェイス
- putty/
  - putty 0.76 を利用した ssh-agent client
- skelton/
  - クライアントスケルトン
  - 何も行わない
- sshagentc/
  - 作成中

# 資料

- SSH Agent Protocol
  - https://datatracker.ietf.org/doc/html/draft-miller-ssh-agent-04
- https://qiita.com/slotport/items/e1d5a5dbd3aa7c6a2a24
