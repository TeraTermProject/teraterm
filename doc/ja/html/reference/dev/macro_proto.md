
# Macro Protocol

## XTYP_EXECUTE

- command
  - command definitions are in[teraterm/common/ttddecmnd.h](https://github.com/TeraTermProject/teraterm/blob/c7ce43608e0e2844e148b88cfadb9a96cebaba01/teraterm/common/ttddecmnd.h#L31)
- ttpmacro -> ttermpro
- packet length limit is MaxStrLen

| pos | len | Description |
|-----|-----|-------------|
| 0   | 1   | command     |
|     | N   | data        |

- command (CmdSendUTF8String/CmdSendBinary/CmdSendCompatString)

| pos | len | Description            |
|-----|-----|------------------------|
| 0   | 1   | command                |
| 1   | 4   | length(little endian)  |
| 5   | N   | data(string or binary) |


## XTYP_POKE

- data(text and binary)
- ttpmacro -> ttermpro

## XTYP_REQUEST

- ttpmacro <- ttermpro
  - Item "DATA"
    - text or binary
  - Item "PARAM"
    - ファイル名的な?
    - 通信の応答


## DDE(DDEML) WIN32 APIs

- DdeAccessData()
  - ハンドル(HDDEDATA)から先頭のポインタとサイズを取得できる
- DdeUnaccessData()
  - DdeAccessData() した物を解除する
- DdeGetData()
  - ハンドル(HDDEDATA)のデータをワークにコピーする
  - 戻り値が実際のサイズ

