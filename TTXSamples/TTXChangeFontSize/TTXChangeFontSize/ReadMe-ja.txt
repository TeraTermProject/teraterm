TTXChangeFontSize -- Tera TermのVTウィンドウにフォントサイズ変更メニューを表示する

機能:
  フォントサイズ変更メニュー

解説:
  フォントサイズ変更メニューが追加されます。

メニューID:
  以下のメニューIDが割り当てられています。
  これらのメニューIDを使って、マクロのcallmenuコマンドやKEYBOARD.CNF設定から
  機能を呼び出せます。

  58101～58120: メニュー項目1～メニュー項目20
  58151: フォントサイズ拡大
  58152: フォントサイズ拡大

設定例(TERATERM.INI):
  [Change Font Size]
  FontSize1 = -15
  FontSize2 = -19

設定例(KEYBOARD.CNF):
  [User keys]
  ; Control-1 -> フォントサイズ拡大
  User1=1026,3,58151
  ; Control-2 -> フォントサイズ縮小
  User2=1027,3,58152

バグ:
  沢山あると思います。

ToDo:
  略
