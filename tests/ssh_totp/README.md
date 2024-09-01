# SSH追加認証テスト環境

開発PC上にテスト用SSHサーバーを起動する

## 参考資料

- [Linux で SSH の 2 要素認証をセットアップする方法](https://ja.linux-console.net/?p=1141)

## 作成済みキーペア

ユーザー'test'用

- tt_test_key,tt_test_key.pub
- password
  - 'pw'
```
$ ssh-keygen -f tt_test_key
Generating public/private ed25519 key pair.
tt_test_key already exists.
Overwrite (y/n)? y
Enter passphrase (empty for no passphrase):
Enter same passphrase again:
Your identification has been saved in tt_test_key
Your public key has been saved in tt_test_key.pub
The key fingerprint is:
SHA256:Cv0t1Dstw6kZ3bUAxRaXRkGYm8NTaCXi+da3vQq+Rxk masaaki@carbon
The key's randomart image is:
+--[ED25519 256]--+
|          ..+O*o |
|         . +Bo+  |
|          ++ =   |
|     .   . o*E   |
|    . . S . +o= .|
|     . + + * = oo|
|      . + X.+ ...|
|         =.+..  .|
|        o  oo... |
+----[SHA256]-----+
```

## TOTP設定

TOTP Token Generator サイト(次のURL)を開いておく
- https://totp.danhersam.com/

次のコマンドを実行(ヒストリに入っているので、上矢印で出てくる)
```
# sudo -u test google-authenticator --time-based --qr-mode=NONE
```
例
```
Your new secret key is: WKCET2ZZESUZNNKC2LWUIZ4OHA
Enter code from app (-1 to skip): 539119
Your emergency scratch codes are:
  56275753
  27179741
  85851173
  47941584
  35726819
Do you want me to update your "/home/test/.google_authenticator" file? (y/n) y
 :
 :
```
- secret key をTOTP Token Generatorサイトに入力(ここでは`WKCET2ZZESUZNNKC2LWUIZ4OHA`)
- サイトに出力された6桁のコードを入力(ここでは'539119')
- `~/.google_authenticator` が生成される
- 残りは 'y' でok

## テスト用sshd 起動

- ポート22を使用していない状態にする
- Windowsの場合
  - [Docker Desktop for Windows](https://www.docker.com/get-started/) をインストールしておく
  - `docker_build.bat` を実行する
- 作成済みテスト用ユーザー
  - user
    - test
  - pw
    - password
- rootでログインした状態になる

- /etc/ssh/sshd_config を調整
  `vi /etc/ssh/sshd_config`
- /etc/ssh/sshd_config を調整して sshd を起動
  `/usr/sbin/sshd -d`
- localhost:22 に接続可能となる

### Tera Term から接続

- `ssh://test@localhost` へ接続
- SSH 認証チャレンジ Password: で
  - `password` と入力
- SSH 認証チャレンジ Verification code: で
  - TOTP Token Generator サイトの 6桁の数字を入力

