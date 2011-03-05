TTXRecurringCommand -- 定期的に指定された文字列を送信する

機能:
  Ayera版Tera Term Pro webにある、Recurring Commandと同等の機能を実現する
  為のTTXです。
  有効にすると、指定した秒数の間キー入力が無いと指定した文字列(コマンド)を
  送信します。
  コマンドに以下の文字列を含めた場合、対応する文字に置き換えられます。
    \n -- 改行(LF)
    \t -- タブ

解説:

設定例:
  [TTXRecurringCommand]
  Enable=on
  Command=date\n
  Interval=300

バグ:
