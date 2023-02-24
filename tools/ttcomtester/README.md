# ttcomtester

- 通信のテストを行うツール
- コマンドラインプログラム
  - cmd などから実行する

## 使い方

コマンドライン

```
ttcomtester [options]
  -h, --help               show this
  -v, --verbose            verbose, 'v' command
  -i, --inifile [inifile]  read inifile, default=ttcomtester.ini
  -d, --device [name]      open device name, ex com2
  -t, --test [name]        run test
  -o, --option [option]
        "BuildComm=baud=9600 parity=N data=8 stop=1"
```

複数のテストを切り替えることができる

- `--test send/receive`
  - 送受信テスト
  - デフォルト(指定しなかったとき)のテスト
- `--test echo`
  - エコーテスト
  - 受信したデータをそのまま送信する
- `--test echo_rts`
  - echo エコーの動作に加えて
  - RTSラインを1秒ごとに0,1を切りかえる
  - 接続先がハードフローを使用しているとき、送信先は1秒ごとに送信しなくなる

## send/receive test (default test)

### 起動例

例1
- `./ttcomtester --device com1 -o "BuildComm=baud=9600 parity=N data=8 stop=1 octs=on rts=on" --verbose`
- com1, (baud=9600 parity=N data=8 stop=1)
- RTS/CTS flow
  - input CTS フロー制御あり
  - output RTS=1
- verbose

例2
- `./ttcomtester --device com1 -o "BuildComm=baud=57600 parity=N data=8 stop=1 octs=on rts=hs" --verbose`
- com2, (baud=57600 parity=N data=8 stop=1)
- RTS/CTS flow
  - input CTS フロー制御あり
  - output RTS=Hand Shake (フロー処理あり)
- verbose

- 注意
  - RTSラインの制御(`r`キー)は、`rts=on` または `rts=off` の時のみ有効
    - ドライバによって異なるかもしれない

### コマンド

ttcomtester はキー入力を受け付け、つぎの2つの状態がある。

- command mode
  - ttcomtester へキーで指示するためのモード
  - ':' キーで send mode へ切り替わる
  - 'o' device open
  - 'c' device open
  - 'q' 終了
  - 'r' RTS=0/1 を切り替える
  - 'l' CTSの状態を表示する
  -  :  実装中…
  - ' ' (スペース) で使えるキーを表示
- send mode
  - 押下したきーコードをそのまま送信するモード
  - ':' キーで command mode へ切り替わる

デバイスをオープンした状態のとき、データを受信すると表示する。

### 受信

オープンしたデバイスにデータが送られてくると画面に表示する。

### 送信

send mode時、押したキーをそのまま送信する。

フロー制御が行われると送信されないこともある

## echo test (--test echo),(--test echo_rts)

起動例
- `ttcomtester --device com1 -o "BuildComm=baud=57600 parity=N data=8 stop=1 octs=on rts=hs" -test echo`

起動(ttcomtesterがRTSを0/1する)
- `ttcomtester --device com1 -o "BuildComm=baud=57600 parity=N data=8 stop=1 octs=on rts=on" -test echo_rts`

### 表示

- `r` or `R`
  - receive datas
- `s` or `S`
  - send datas
- `t`
  - RTS=0
- `T`
  - RTS=1

### キー

`q` 終了

# 参考

## ヌルモデム

- [ヌルモデム(wikipedia)](https://ja.wikipedia.org/wiki/%E3%83%8C%E3%83%AB%E3%83%A2%E3%83%87%E3%83%A0)

## 仮想comソフト

- HHD SOFTWARE / Virtual Serail Ports
  - https://www.hhdsoftware.com/virtual-serial-ports
    - インストールしてある程度経過すると、制限ありの無料版として継続使用できる
  - 1台のPCで接続された2ポートを作成できる

## RTS/CTS

- [RTSをめぐる混乱](https://lipoyang.hatenablog.com/entry/20130530/p1)

## Terminal soft

- HILGRAEVE / HYPERTERMINAL
  - HyperTerminal Free Trial for Windows 11, 10, 8, 7, Vista, and XP
  - https://www.hilgraeve.com/hyperterminal-trial/
  - A free 30 day trial

- Realterm
  - https://realterm.sourceforge.io/

## Microsoft

- Serial Communications in Win32
  - https://learn.microsoft.com/en-us/previous-versions/ms810467(v=msdn.10)?redirectedfrom=MSDN

- シリアルAPIs
  - https://learn.microsoft.com/ja-jp/windows/win32/devio/communications-functions
