# ttcomtester

- 通信のテストを行うツール
- コマンドラインプログラム
  - cmd などから実行する

## 動作確認

- シリアルをクロス接続する
  - 同一PCでも別PCでもok
  - HHD Virtual Serail Ports の Local Bridges を作ってテストできる
    - https://www.hhdsoftware.com/
    - インストールしてある程度経過すると、制限あり無料版として継続使用できる
- 受信側(例com1)を起動
  - `./ttcomtester --device com1 --rts on --verbose` 起動
    - `Rts Flow control` が 1 になっていることを確認
  - `o` を押す
    - 'open com=...'と表示、read待ちになる
  - `r` を押すごとに、RTSラインをon/offできる
- 送信側(例com2)を起動
  - `./ttcomtester --device com2 --rts hs --verbose` 起動
    - `Rts Flow control` が 2 になっていることを確認
  - `o` を押す
    - 'open com=...'と表示、read待ちになる
  - `:` を押す
    - send mode に切り替わる(send/command 切り替え)
  - `:` 以外のキーを押すと、受信側に送信される
- 受信側でも send mode に切り替えて相互に送受信できることを確認する
- 送信側で `s` 32x1024byteデータ送信
  - うまく送信できるはず

## RTS/CTSハードフローテスト

- 受信側は
  - RTS/CTSフローは使用していない状態で起動している
    - `--rts on` は RTSライン=1 で初期化する
  - command mode で、`r` 毎に RTS=0/1 切り替えられる
- フロー制御が行われているか確認
  - 受信側でRTS=0/1を切り替えて、送信側から送信
    - 送信側で send mode にして、適当なキーを押して送信
  - 送信側から 32x1024byteのデータを送信、受信中にRTS=0/1を切り替え、

## Tera Term で RTS/CTSテスト

- 送信側を Tera Term に切り替える
- パラメータは "baud=9600 parity=N data=8 stop=1", フロー RTS/CTS
- 受信側が RTSライン=0 としていても Tera Term は送信してくる

# 参考

## RTS/CTS

- [RTSをめぐる混乱](https://lipoyang.hatenablog.com/entry/20130530/p1)

## HyperTerminal

- HyperTerminal Free Trial for Windows 11, 10, 8, 7, Vista, and XP
  - https://www.hilgraeve.com/hyperterminal-trial/
  - A free 30 day trial

