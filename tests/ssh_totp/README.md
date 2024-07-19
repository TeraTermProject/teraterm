# SSH追加認証テスト環境

開発PC上にテスト用SSHサーバーを起動する

## 参考資料

- [Linux で SSH の 2 要素認証をセットアップする方法](https://ja.linux-console.net/?p=1141)

## テスト用サーバー起動

- docker desktop をインストールしておく
- ポート22を使用していない状態にする
- `docker_build.bat` を実行する
- ユーザー
  - user
    - test
  - pw
    - password
- rootでログインした状態になる

## TOTP設定

TOTP Token Generator サイト(次のURL)を開いておく
- https://totp.danhersam.com/

次のコマンドを実行(ヒストリに入っているので、上矢印で出てくる)
```
# google-authenticator --time-based --qr-mode=NONE
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

`exit`でtestユーザーをログアウト、rootに戻る

## sshd 起動

```
$ /usr/sbin/sshd -d
```

サーバーの準備完了
localhost:22 に接続可能となる

### Tera Term から接続

- `ssh://test@localhost` へ接続
- `キーボードインタラクティブ認証を使う` を選択
- SSH 認証チャレンジ Password: で
  - `password` と入力
- SSH 認証チャレンジ Verification code: で
  - TOTP Token Generator サイトの 6桁の数字を入力
