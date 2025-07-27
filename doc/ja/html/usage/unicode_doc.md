
Unicodeの実装について。
[wiki](https://github.com/TeraTermProject/teraterm/wiki/TODO-Unicode)から移動。

markdownからhtmlへの変換
```
pandoc unicode_doc.md -t html -o unicode_doc.html
```

### Unicode(内部文字コード)

Tera Termが認識するUnicode仕様について。

Tera Term内部では認識しますが、
Windowsのバージョンや使用フォントによって実際の表示は変化します。
(カラー絵文字は未サポートです)

- 2cell(全角)の行末処理
  - 2cellより大きな文字の行末処理は未テスト
- 各文字の文字幅
  - "A"(U+0041)は1cell、"あ"(U+3042)は2cellなど、各文字ごとに文字幅が異なっている
  - Unicodeの仕様を元にしている
    - [EastAsianWidth.txt](http://www.unicode.org/Public/UCD/latest/ucd/EastAsianWidth.txt)
    - Ambiguous character
      - 場合によって文字幅が変化する文字
      - 文字幅の設定可
  - 絵文字
    - 文字幅設定可
    - 絵文字幅上書き設定OFF時
      - Unicode文字幅
    - 絵文字幅上書き設定ON時
      - [emoji-data.txt](https://www.unicode.org/Public/UCD/latest/ucd/emoji/emoji-data.txt) をもとに絵文字を判定
      - 設定が2cell設定の時
        - すべてを2cellとして扱う
      - 1cell設定時
        - U+1f000未満は1cell
        - U+1f000以上は2cell
- 結合文字
   - [UnicodeData.txt](https://www.unicode.org/Public/UCD/latest/ucd/UnicodeData.txt)をもとに決定
   - Bengali, Khmer, Tibetan など
- VariationSelector(異体字セレクタ)
  - U+FE00...FE0F    VARIATION SELECTOR-1...16
    - U+FE0E VARIATION SELECTOR-15 (TPVS)
      - テキストスタイル
    - U+FE0F VARIATION SELECTOR-16 (EPVS)
      - 絵文字スタイル
  - U+E0100...E01EF  VARIATION SELECTOR-17...256
    - 17番以降は漢字の異体字用
- EMOJI MODIFIER
  - U+1F3FB     EMOJI MODIFIER FITZPATRICK TYPE-1-2
  - U+1F3FC...F EMOJI MODIFIER FITZPATRICK TYPE-3...6
- Zero Width Joiner(ZWJ,ゼロ幅接合子)
  - U+200D
- Zero Width non-joiner(ZWNJ, ゼロ幅非接合子)
  - U+200C

#### 例/結合文字

```
$ echo -e '\U0061\U0041'
aA
$ echo -e '\U0061\U0302\U0041\U0302'
âÂ
$ echo -e '\U037B'
ほ
$ echo -e '\U037B\309A'
ぽ
$ echo -e '\U0938\U094d\U0924\U0947'
स्ते
$ echo -e '\U0ab8\U0acd\U0aa4\U0ac7'
સ્તે
```

#### 例/漢字異体字

```
$ echo -e '\u845B'
葛
$ echo -e '\u845B\UE0100'
葛󠄀
$ echo -e '\u845B\UE0101'
葛󠄁
```
#### 例/絵文字スタイル(presentation style)

未指定時はシステムによって異なります。
(カラー絵文字の描画は未サポートです)

```
$ echo -e '\U1F308'
🌈
$ echo -e '\U1F308\UFE0E'
🌈︎
$ echo -e '\U1F308\UFE0F'
🌈️
```

#### 例/EMOJI MODIFIER

色が変化します。
(カラー絵文字の描画は未サポートです)

```
$ echo -e '\U1F476'
👶
$ echo -e '\U1F476\U1F3FB'
👶🏻
$ echo -e '\U1F476\U1F3FC'
👶🏼
$ echo -e '\U1F476\U1F3FD'
👶🏽
$ echo -e '\U1F476\U1F3FE'
👶🏾
$ echo -e '\U1F476\U1F3FF'
👶🏿
```

#### 例/Zero Width Joiner

```
$ echo -e '\U1F468\U200D\U1F469\U200D\U1F466\U200D\U1F467'
👨‍👩‍👦‍👧
```



### 画面描画

- WindowsのAPIを使用して描画を行う
  - 独自の描画は行っていない
  - Windowsのバージョン,使用フォントによって結果が異なる
  - 使用するフォントファイルによって結果が異なる
- Unicode系APIと非Unicode系APIのどちらでも利用可能
  - 設定で選択可
    - 9x系でなければ通常はUnicode系APIを使用する
    - ExtTextOutW() 又は ExtTextOutA()
  - 9x系のExtTextOutW() について
     - 存在するが動作がNT系と異なるようだ
       - [seclan のほえほえルーム](http://seclan.dll.jp/dtdiary/2010/dt20100310.htm)
- カラー絵文字 未実装
  - DirectWrite というDirect Xの機能を使う
  - Windows8以降のサポートとなる

### 表示,機能

Unicode化されている機能

- ダイアログ、メニュなどの文字表示
- URL
  - 強調表示
  - クリッカブルURL
    - クリックでブラウザなど実行
    - アルファベットのみ、漢字などを使ったURLは認識しない
- クリップボード
  - クリップボードへセット
    - [Synthesized Clipboard Formats](https://docs.microsoft.com/en-us/windows/win32/dataxchg/clipboard-formats)を利用
  - クリップボードからゲット
  - 9x系でのサポートがどうなっているか未検証
- ファイルドロップ
  - ファイル名のペースト
  - ファイルの送信
    - バイナリ
    - テキスト
    - ファイルの文字コードの自動判定
    - [ttlファイルのエンコード](https://teratermproject.github.io/manual/5/ja/macro/syntax/file.html)のファイル読み込みと同じ
  - scp
    - ファイル名は UTF-8でホストに送る
- ファイル送信
  - バイナリ
  - テキスト
    - ファイルの文字コードの判定
    - [ttlファイルのエンコード](https://teratermproject.github.io/manual/5/ja/macro/syntax/file.html)のファイル読み込みと同じ
- ファイル転送
  - ローカルのファイル名は Unicodeに対応
  - 送信するファイル名は ANSIで送る(UTF-8/ANSI切り替えUIがほしい)
- ファイル受信(ログ)
  - 文字コード選択できるUIを追加
- 印字
  - [ファイル]/[印刷] Unicode対応
  - エスケープシーケンスによる印刷
    - 印刷
      - (△) 難しそう、ANSIで実装しておく
    - パススルー印字
      - 文字コードを何で送るか設定が必要
- ttxsshのduplicate session
- パス、ファイル名など
  - 設定ファイル(iniファイル)のデフォルト保存場所のファイル名など
- iniファイルのパースなど

### plugin

- 作業中…
- Tera Term 4 以前とのバイナリ互換性は失われる
- 新しい機能は入れず、なるべく変更なくソース再コンパイル(ソース互換)で使えるようにする方針

### 追加設定

- Debug設定
  - 文字情報ポップアップ
    - on/off
    - キー設定

### 9x対応

- 最新OSで動作することを優先、低プライオリティで対応
- 非Unicode系APIしかない場合は、Unicode APIの動作を内部でエミュレートする
  - ある程度は9xでも動作
- ツール/ライブラリ/OSの対応
  - 古いOSで動作させるためには古いツールセットを使用する必要がある
  - 新しいバージョンのライブラリは古いツールセットではビルドできないことがある
  - 新しいバージョンのライブラリは古いOSでは利用できないことがある

### 実装未検討

- 左横書き
  - アラビア語など

