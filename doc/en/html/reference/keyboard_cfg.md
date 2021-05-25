# キーボード設定ファイルについて

## キーボード設定ファイルの形式

キーボード設定ファイルの中で用いられる PC key code は PC の各キーまたはキーの
組み合わせに対応した数値で、使用するキーボードによって異なります。

キーボード設定ファイルには次の6つのセクションが存在します。

- [VT editor keypad]
- [VT numeric keypad]
- [VT function keys]
- [X function keys]
- [Shortcut keys]
- [User keys]

### [VT editor keypad] セクション

VT 端末のエディターキーを PC キーに割り当てます。

	形式:
		<VT editor key name>=<PC key code>

	<VT editor key name>
		Up, Down, Right, Left, Find, Insert, Remove, Select,
		Prev, Next

	<PC key code>
		PC key code (10進数)

	例:
		Up=328

### [VT numeric keypad] セクション

VT端末の数値キーを PC キーに割り当てます。

	形式:
		<VT numeric key name>=<PC key code>

	<VT numeric key name>
		Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8,
		Num9, NumMinus, NumComma, NumPeriod, NumEnter, PF1,
		PF2, PF3, PF4

	<PC key code>
		PC key code (10進数)

	例:
		Num0=82

#### 注意

VT 端末ではメインキーパッドの "Enter" キーと数値キーパッドの
"Enter" キーが違う文字列を送出するモードがあるので、"NumEnter=" の
後にはメインキーの "Enter" の PC key code を書かないでください。
メインキーパッドの "Enter" と数値キーパッドの "Enter" が同じ
PC key code を発生するキーボード(例えば PC9801 キーボード)の場合は
注意が必要です。

### [VT function keys] セクション

VT 端末のファンクションキーを PC キーに割り当てます。

	形式:
		<VT function key name>=<PC key code>

	<VT function key name>
		(VT 端末のファンクションキー)
		Hold, Print, Break, F6, F7, F8, F9, F10, F11, F12,
		F13, F14, Help, Do, F17, F18, F19, F20
		(VT 端末の"ユーザー定義キー")
		UDK6, UDK7, UDK8, UDK9, UDK10, UDK11, UDK12, UDK13,
		UDK14, UDK15, UDK16, UDK17, UDK18, UDK19, UDK20

	<PC key code>
		PC key code (10進数)

	例:
		F6=64

### [X function keys] セクション

Xterm の F1-F5 キー、およびバックタブキーを PC キーに割り当てます。

	形式:
		<Xterm function key name>=<PC key code>

	<Xterm function key name>
		XF1, XF2, XF3, XF4, XF5, XBackTab

	<PC key code>
		PC key code (10進数)

	例:
		XF1=59

### [Shortcut keys] セクション

Tera Term の機能をPC キーに割り当てます。

	形式:
		<Shortcut key name>=<PC key code>

	<Shortcut key name>	機能
	---------------------------------------------------------
	EditCopy		[Edit] Copy コマンド
	EditPaste		[Edit] Paste コマンド
	EditPasteCR		[Edit] Paste<CR> コマンド
	EditCLS 		[Edit] Clear screen コマンド
	EditCLB 		[Edit] Clear buffer コマンド
	ControlOpenTEK		[Control] Open TEK コマンド
	ControlCloseTEK 	[Control] Close TEK コマンド
	LineUp			一行スクロールアップ
	LineDown		一行スクロールダウン
	PageUp			一ページスクロールアップ
	PageDown		一ページスクロールダウン
	BuffTop 		バッファー先頭へスクロール
	BuffBottom		バッファー最後へスクロール
	NextWin 		次の Tera Term ウィンドウへ移動
	PrevWin 		前の Tera Term ウィンドウへ移動
	NextShownWin 		次の最小化されていない Tera Term ウィンドウへ移動
	PrevShownWin 		前の最小化されていない Tera Term ウィンドウへ移動
	LocalEcho		Local echo を on/off する

	<PC key code>
		PC key code (10進数)

	例:
		LineUp=1352

### [User keys] セクション

ユーザーキーと、そのキーを押したときに実行される機能
(文字列の送出、マクロファイルの実行、メニューコマンドの実行)を
定義します。

	形式:
		<User key name>=<PC key code>,<Control flag>,<文字列>
		<User key name>=off

	<User key name>
		User1, User2, User3,...., User99
		最大99個まで設定可能

	<PC key code>
		PC key code (10進数)

	<Control flag>
		キーを押したときに <文字列> をどのように取り扱うかを指定
		するフラグ。
			0	<文字列>をそのまま(8bit/文字として)送出する。
			1	<文字列>に含まれる文字や改行コードを
				Tera Term の設定にあわせて変換し、変換
				された文字列を送出する。
			2	<文字列>のファイル名のマクロファイルを
				実行する。
			3	メニュー ID <文字列> で指定される
				Tera Term のメニューコマンドを実行する。

	<文字列>:
		<Control flag> が 0 または 1 の場合、キーを押したときに
		送出される文字列。表示不可能な文字(制御文字等)はその
		文字コードを $ と2文字の16進数で表現する。
		(例: CR 文字は '$0D')。"$" そのものは "$24" で表現する。
		Tera Term 内部では設定ファイルはUTF-16(16bit/文字)で処理している。
		<Control flag> が 0 の場合、8bit/文字として処理する。
		U+0000..U+00FFは$00..$FFとしてそのまま送信する。
		それ以外は$FFとして送信する。
			U+0000..U+007F=基本ラテン文字
			U+0080..U+00FF=ラテン1補助
		<Control flag> が 1 の場合、
		Unicode文字列として処理する。

		<Control flag> が 2 の場合、
		実行されるマクロファイルのファイル名。

		<Control flag> が 3 の場合、
		実行されるメニューコマンドのメニュー ID (数字)。
		メニュー IDについては「メニュー ID 表」参照。

	例:
		User1=1083,0,telnet myhost
		User2=1084,0,$0D$0A
		User3=1085,1,こんにちは。
		User4=1086,2,test.ttl
		User5=1087,3,50110

## 注意

1つの PC key code はキーボード設定ファイルの中で一回だけ使用することが
できます。もし、1つの PC key code を複数のキー定義で使用した場合、
Tera Term がキーボード設定ファイルを読み込んだときに、
"Key code XXX is used more than once" という警告メッセージが表示されます。
この場合ある一つのキー定義だけが有効になり、その他は無視されます。

あるキー設定項目にどの PC キーも割り当てたくない場合は、以下のように
PC key code の代わりに "off" を指定してください。

    EditCopy=off

## 可能なキーの組み合わせ

キーボード設定ファイルで設定可能な PC のキーは, KEYCODE.EXE で PC key code が
表示されるキーです。単一のキーだけでなく、Ctrl, Shift, Alt を用いたキーの組み
合わせでも PC key code を表示させることができます。可能な組み合わせを
以下に示します。

	Shift+key
	Ctrl+key
	Shift+Ctrl+key
	Shift+Alt+key
	Ctrl+Alt+key
	Shift+Ctrl+Alt+key

Tera Term や Windows のショートカットキーに割り当てられているキーの組み合わせ
(例えば Alt+key など)はキーボード設定ファイルで指定できません。

ただし、Altキーをメタ・キーとして使う設定にしている場合(設定->キーボードにある
Metaキーにチェックをいれている状態)はAlt+keyも指定する事ができます。
キーコードは KEYCODE.EXE で調べられますが、単体のAltキーとの組合せには
対応していません。Altキーとの組合せでのキーコードを調べるには、単独での
キーコードを調べて、その値に2048を足してください。

たとえば、Alt+Vのキーコードは V が 47 なので、2095 となります。

    ; Shift + Insert
    EditPaste=850

これを例えば EditPaste=2095 に変更すれば、Metaキーをパススルーにしていても
Alt+Vで張り付けができるようになります。

Shift+Insertを残したままAlt+Vでの張り付けを行いたい場合は、KEYBOARD.CNF の
[User keys]セクションに以下の設定を追加します。

    User1=2095,3,50230

## Q & A

- Q. Tera Term を起動するたびに "Key code XXX is used more than once" という
   メッセージがでる。
- A. 「注意」を参照。

- Q. PC の F1 キーを VT100 の PF1 キーとして使いたい。以下のように設定したが
   うまくいかない。

	[VT function keys]
	F1=PF1		(これはまちがい)

- A. 左辺は **PC** のキーの名前ではなく **VT端末** のキーの名前を指定しなけ
   ればなりません。また、右辺にはキーの名前ではなく、キーコードを指定して
   ください。
   また、キー設定を変えるときは、キーコードの重複使用をしないようにして
   ください。

   以下のように設定してください。

	[VT function keys]
	PF1=59			(59 は F1 キーのキーコード)
	[X function keys]
	;XF1=59			(キーコード 59 の重複使用をさける)
	XF1=off			("off" で置き換える)

- Q. F1 キーを押したときにエスケープシーケンス ESC [ A を送出するように
   するための設定方法は?
- A. ユーザーキーを使えば、好きな文字列を送出することができます。
   くわしくは、「3.1 キーボード設定ファイルの形式」を読んでください。
   また、キー設定を変えるときは、キーコードの重複使用をしないようにして
   ください(「3.2 注意」を参照)。

   以下のように設定してください。

	[X function keys]
	;XF1=59			(F1 キーのキーコード 59 の重複使用をさける)
	XF1=off			("off" で置き換える)
	[User keys]
	User1=59,0,$1B[A		(ESC の ASCII コードは $1B)

