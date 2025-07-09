TTXViewMode -- support view only mode.

Features:
  TTXViewMode adds a "View Mode" option to the control menu of Tera Term.
  When this menu item is selected, only data reception (display) is possible,
  and data transmission via keyboard input is disabled.
  If selected again, the mode will be released if no password is set;
  however, if a password is set, it will not be released until the correct password is entered.
  Password settings can be configured in the settings menu under "ViewMode password".

Description:
  This is a sample that hooks into data transmission.
  When activated, it discards all transmitted data that is passed to it.

Bugs:
  * It stops the sending of Telnet Keep-Alive packets.
  * It does not allow saving the password in the configuration file.

ToDo:
  * Enable saving the password in the configuration file.

Other:
  This plugin was developed based on the following request:
  https://ja.osdn.net/ticket/browse.php?group_id=1412&tid=8850
