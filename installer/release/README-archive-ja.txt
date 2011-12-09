アーカイブ版の注意

- 通常はインストーラにより Tera Special フォントがインストールされます。
  必要な場合は TSPECIAL1.TTF を手動でインストールしてください。

- TERATERM.INI のなかには、インストーラで選択することによりに設定される
  値がありますが、アーカイブ版はそうではありません。必要な場合は手動で
  設定してください。
  メニューやダイアログの日本語を有効にするには、TERATERM.INI の
  UILanguageFile を lang\Japanese.lng に変更してください。

; User interface language file that includes message strings.
; Tera Term uses English message when the filename is not specified.
UILanguageFile=lang\Japanese.lng

- Windows 7 のジャンプリストが有効になっているとファイル(%AppData%\
  Microsoft\Windows\Recent\CustomDestinations)に情報が記録されます。
  この動作が望ましくない場合は TERATERM.INI の JumpList を
  off に変更してジャンプリストを無効にしてください。

- インストーラの標準インストールでは含まれない多くの追加プラグインが
  同梱されています。不要なものは削除・リネームしてください。

- LogMeTT, TTLEdit は含まれていません。必要な場合は入手してください。
  http://logmett.com/freeware/LogMeTT.php
  http://logmett.com/freeware/TTLEdit.php

