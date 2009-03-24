TTXResizeWin -- Tera TermのVTウィンドウのサイズを80x24に変更する

機能:
  Tera Termのメニューに“80x24”という項目を追加します。
  この項目を選択すると、Tera TermのVTウィンドウのサイズが80x24に変更されます。

解説:
  メニュー項目が選択された時は、設定情報の端末サイズを変更します。
  しかしこれだけでは実際のウィンドウサイズは変更されません。
  そこで、端末設定ダイアログでOKボタンを押した後にウィンドウサイズの変更の
  処理が行われるのを利用しています。

  具体的な処理は、pvar->ReplaceTermDlgがTRUEの時はTTXGetUIHooksで“端末設定
  ダイアログ”を“pvar->ReplaceTermDlgをFALSEにする他はなにもしない関数”に
  置き換えます。これによって、pvar->ReplaceTermDlgをTRUEにした後の最初の
  端末設定ダイアログの呼び出しはダイアログを表示せずにウィンドウサイズの
  変更処理が行われるようになります。
  そしてメニュー項目が選択された時に、設定情報を変更すると同時に
  pvar->ReplaceTermDlgをTRUEにして端末設定ダイアログを呼び出します。

その他:
  「時にはサイズを変更する時があるが、普段は80x24にするのが正しいTerminal
  emulatorの使い方だ」という意見の人(*1)にお勧めします。
  「80x24以外のサイズでTerminal emulatorを使うのは間違っている」という意見の
  人には、TTXFixedWinSizeをお勧めします。

  *1:
  このプラグインを作ったきっかけは以下の記事です。
  http://groups.google.co.jp/group/fj.comp.security/browse_thread/thread/34cb5ba5ca18d4d8/fada3bf32e29de9c?#fada3bf32e29de9c
