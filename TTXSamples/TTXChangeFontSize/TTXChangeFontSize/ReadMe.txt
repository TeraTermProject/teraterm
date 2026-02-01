TTXChangeFontSize -- Change font size

Feature:
  Change font size.

Description:

Menu ID:
  58101-58120: Menu No.1 - Menu No.20
  58151: Increase font size.
  58152: Decrease font size.

Example configuration (TERATERM.INI):
  [Change Font Size]
  FontSize1 = -15
  FontSize2 = -19

Example configuration (KEYBOARD.CNF):
  [User keys]
  ; Control-1 -> Increase font size.
  User1=1026,3,58151
  ; Control-2 -> Decrease font size.
  User2=1027,3,58152

Bugs:

ToDo:
