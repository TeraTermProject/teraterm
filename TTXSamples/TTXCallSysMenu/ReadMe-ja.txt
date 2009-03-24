TTXCallSysMenu -- システムメニューをキーボード/マクロから呼ぶ手段を提供する。

機能:
  KEYBOARD.CNFの設定やマクロのcallmenuコマンドで、システムメニューを開く事が
  出来るようになります。

  KEYBOARD.CNFに以下の設定を追加すると、Control+Alt+Spaceでシステムメニューが
  開くようになります。

  [User keys]
  ; Ctrl+Alt+Space -> System Menu
  User1=3129,3,39393

  マクロでは、callmenu 39393 を実行するとシステムメニューが開きます。

解説:
  メニューID 39393が呼ばれたら、Tera Termのウィンドウにシステムメニューを
  開くためのメッセージをPostしているだけです。

元ネタ:
  http://pc11.2ch.net/test/read.cgi/unix/1225116847/74
  bash等でAlt+f等を使えるようにするためには"設定"->"キーボード"->"Metaキー"を
  Onにする必要がありますが、Alt+Spaceでシステムメニューが開けなくなります。
  それを補完するためにこのTTXは作られました。

ToDo:
  TTXAlwaysOnTop辺りと機能を統合した方がいいかもしれません。
