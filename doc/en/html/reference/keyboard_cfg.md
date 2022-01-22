# ABOUT THE KEYBOARD SETUP FILE

## FORMAT OF THE KEYBOARD SETUP FILE

Each key or key combination has a unique key code, which is called
"PC key code".

The keyboard setup file has six sections.

- [VT editor keypad]
- [VT numeric keypad]
- [VT function keys]
- [X function keys]
- [Shortcut keys]
- [User keys]

### [VT editor keypad] section

In this section, VT editor keys are assigned to PC keys.

	Format:
		<VT editor key name>=<PC key code>

	where:

	<VT editor key name>
		Up, Down, Right, Left, Find, Insert, Remove, Select,
		Prev, Next

	  <PC key code>
		PC key code (decimal number)

	Example:
		Up=328

### [VT numeric keypad] section

In this section, VT numeric keys are assigned to PC keys.

	Format:
		<VT numeric key name>=<PC key code>

	where:

	  <VT numeric key name>
		Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8,
		Num9, NumMinus, NumComma, NumPeriod, NumEnter, PF1,
		PF2, PF3, PF4

	  <PC key code>
		PC key code (decimal number)

	Example:
		Num0=82

#### 注意

VT 端末ではメインキーパッドの "Enter" キーと数値キーパッドの
"Enter" キーが違う文字列を送出するモードがあるので、"NumEnter=" の
後にはメインキーの "Enter" の PC key code を書かないでください。
メインキーパッドの "Enter" と数値キーパッドの "Enter" が同じ
PC key code を発生するキーボード(例えば PC9801 キーボード)の場合は
注意が必要です。

### [VT function keys] section

In this section, VT function keys are assigned to PC keys.

	Format:
		<VT function key name>=<PC key code>

	where:

	  <VT function key name>
		(Function keys of VT terminal)
		Hold, Print, Break, F6, F7, F8, F9, F10, F11, F12,
		F13, F14, Help, Do, F17, F18, F19, F20
		("User defined keys" of VT terminal)
		UDK6, UDK7, UDK8, UDK9, UDK10, UDK11, UDK12, UDK13,
		UDK14, UDK15, UDK16, UDK17, UDK18, UDK19, UDK20

	  <PC key code>
		PC key code (decimal number)

	Example:
		F6=64

### [X function keys] section

In this section, Xterm F1-F5 keys and BackTab key are assigned to PC keys.

	Format:
		<Xterm function key name>=<PC key code>

	where:

	  <Xterm function key name>
		XF1, XF2, XF3, XF4, XF5, XBackTab

	  <PC key code>
		PC key code (decimal number)

	Example:
		XF1=59

### [Shortcut keys] section

In this section, Tera Term functions are assigned to PC keys.

	Format:
		<Shortcut key name>=<PC key code>

	where:
	  <Shortcut key name>	Function
	  --------------------------------------------
	  EditCopy		[Edit] Copy command
	  EditPaste		[Edit] Paste command
	  EditPasteCR		[Edit] Paste<CR> command
	  EditCLS		[Edit] Clear screen command
	  EditCLB		[Edit] Clear buffer command
	  ControlOpenTEK	[Control] Open TEK command
	  ControlCloseTEK	[Control] Close TEK command
	  LineUp		Scrolls up screen by 1 line
	  LineDown		Scrolls down by 1 line
	  PageUp		Scrolls up by 1 page
	  PageDown		Scrolls down by 1 page
	  BuffTop		Scrolls screen to buffer top
	  BuffBottom		Scrolls screen to buffer bottom
	  NextWin		Moves to the next Tera Term window
	  PrevWin		Moves to the previous Tera Term window
	  NextShownWin		Moves to the next Tera Term window (except minimized)
	  PrevShownWin		Moves to the previous Tera Term window (except minimized)
	  LocalEcho		Toggles the local echo status

	  <PC key code>
		PC key code (decimal number)

	Example:
		LineUp=1352

### [User keys] section

This section defines user keys for functions, sending a character
string, executing a macro file or executing a menu command.

	Format:
		<User key name>=<PC key code>,<Control flag>,
				<Character string>

	where:

	  <User key name>
		User1, User2, User3,...., User99
		Maximum number of user keys is 99. 
		For example, if you want to define ten user keys,
		you must use the first ten names, from "User1" to "User10".

	  <PC key code>
		PC key code (decimal number)

	  <Control flag>
		Control flag which specifies how <character string>
		is treated when the PC key is pressed.
			0	<Character string> is sent as it is.
			1	New-line codes in <Character string>
				are converted by Tera Term and
				the converted string is sent.
			2	A macro file which has the name of
				<Character string> is executed.
			3	A Tera Term menu command specified
				by the menu ID <Character string> is
				executed.

	  <Character string>
		If <Control flag> is 0 or 1, <Character string>
		represents the character string to be sent.
		A non-printable character (control character) in
		the string can be expressed as a "$" and ASCII code
		in two-character hex number. For example, CR character
		is expressed as "$0D". "$" itself is expressed as "$24".
		See "Appendix A  ASCII CODE TABLE".

		If <Control flag> is 2, <Character string> specifies
		the macro file name to be executed.

		If <Control flag> is 3, <Character string> is the menu
		ID which specifies the menu command to be executed.
		The menu ID should be expressed as a decimal number.
		See "Appendix B  LIST OF MENU IDs".

	Example:
		User1=1083,0,telnet myhost
		User2=1084,0,$0D$0A
		User3=1085,1,$0D
		User4=1086,2,test.ttl
		User5=1087,3,50110

## NOTE

You can use a PC key code only once in the setup file.
If you use a PC key code for multiple key assignments,
the warning message "Key code XXX is used more than once" is
displayed when the file is loaded by Tera Term. In this case,
one of the assignments becomes effective and others are ignored.

If you don't want to assign a key item to any PC key,
use the word "off" like the following:

EditCopy=off

## KEY COMBINATIONS

The following key combinations are acceptable to Tera Term and KEYCODE.EXE:

	Shift+key
	Ctrl+key
	Shift+Ctrl+key
	Shift+Alt+key
	Ctrl+Alt+key
	Shift+Ctrl+Alt+key

Since some combinations (such as Alt+key) are used as shortcut keys of
Tera Term and Windows, they don't have PC key codes and can't be specified
in the keyboard setup file.

[NOTE]
You can specify `Alt+key' combination when Alt key uses meta key. Check
`Meta key' under Keyboard of Setup menu. The key code can be obtained by
KEYCODE.EXE, unfortunately the key code of `Alt+key' combination can not
be obtained. Also, you obtain the key code and add 2048 on the value.

For example, the `V' key code is 47, also `Alt+V' key code is 2095.

; Shift + Insert
EditPaste=850

If you change above entry to `EditPaste=2095', you can paste by using
`Alt+V' on condition that Meta key is pass through.

You add in the following entry to [User keys] section in KEYBOARD.CNF if
you wan to paster by using `Alt+V' remaining `Shift+Insert' function.

User1=2095,3,50230

## Q & A

- Q. Every time I run Tera Term, the warning message
   "Key code XXX is used more than once" is displayed.
- A. See "3.2 NOTE".

- Q. I want to use the PC "F1" key as the VT100 PF1 key. I edit the 
   keyboard setup file like the following but it does not work:

	[VT function keys]
	F1=PF1		(This is wrong.)

- A. The left hand side can not be the name of a **PC key** but
   the name of a **VT terminal key**. The right hand side can not be
   the name of a key but a PC key code.
   See "3.1 FORMAT OF THE KEYBOARD SETUP FILE".
   You should also be careful not to specify a PC key code more than
   once in the setup file (see "3.2 NOTE").

   Edit the keyboard setup file like the following:

	[VT function keys]
	PF1=59			(59 is the keycode for the F1 key.)
	[X function keys]
	;XF1=59			(Avoid specifying 59 twice.)
	XF1=off			(Replace 59 by "off".)

- Q. How to edit the keyboard setup file to assign the F1 key
   for sending the escape sequence "ESC [ A"?
- A. You can send any character string by using a user key.
   See "3.1 FORMAT OF THE KEYBOARD SETUP FILE".
   You should also be careful not to specify a PC keycode more than
   once in the setup file (see "3.2 NOTE").

   Edit the keyboard setup file like the following:

	[X function keys]
	;XF1=59			(59 is the keycode for the F1 key.)
					(Avoid specifying 59 twice.)
	XF1=off			(Replace 59 by "off".)
	[User keys]
	User1=59,0,$1B[A		(The ASCII code for ESC is $1B.)
