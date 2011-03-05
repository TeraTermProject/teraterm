TTXFixedWinSize -- Tera Term VT window changing fixed 80x24.

Feature:
  TTXFixedWinSize plugin makes Tera Term VT window sizing lock up. The fixed size is 80x24.

Description:
  When Tera Term startup and read the configuration, this plugin hooks ReadIniFile function by using TTXGetSetupHooks. Sure, the plugin changes the terminal size after reading the configuration.

  端末設定ダイアログによるサイズの変更は、TTXGetUIHooksで端末設定ダイアログを
  フックして、ダイアログ終了時に端末サイズを書き換えています。

  マウスでの枠のドラッグや最大化ボタン、制御シーケンスによる変更に対しては、
  TTXSetWinSizeの中で元のサイズにリサイズしなおしています。この時のリサイズ
  処理に関してはTTXResizeWinの解説を参照してください。

  TTXResizeWin/TTXResizeMenuによるサイズ変更は実質端末設定ダイアログによる
  ものと同じなので、ORDERをTTXResizeWin/TTXResizeMenuより大きくして後に実行
  されるようにする事で対応しています。

Bug:
  ・どこにも接続していない状態では、マウス等でサイズ変更ができてしまいます。
  ・マウス等によるサイズ変更時は、画面がクリアされたり、接続先にSIGWINCHが
    送られたりする事があります。
  ・マウスで上辺や左辺の枠をドラッグしてサイズを変更しようとした時は、
    ウィンドウの位置が移動してしまいます。

Miscellaneous:
  このプラグインは「80x24以外のサイズでTerminal emulatorを使うのは間違って
  いる」という意見の人(*1)にお勧めします。
  「時にはサイズを変更する時もあるけれど、普段は80x24で使いたいよね」という
  意見の人には、TTXResizeWin/TTXResizeMenuの使用をお勧めします。

  *1:
  元々はTTXResizeWin/TTXResizeMenuの作成後に、冗談で思いついたプラグインです。
