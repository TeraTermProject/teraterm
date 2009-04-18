#define AppName "Tera Term"
#define AppVer "4.62"
#define snapshot GetDateTimeString('yyyymmdd_hhnnss', '', '');

[Setup]
AppCopyright=TeraTerm Project
AppName={#AppName}
#ifndef snapshot
AppVerName={#AppName} {#AppVer}
#else
AppVerName={#AppName} {#AppVer}+ snapshot-{#snapshot}
#endif
LicenseFile=release\license.txt
DefaultDirName={pf}\teraterm
DefaultGroupName={#AppName}
ShowLanguageDialog=yes
AllowNoIcons=true
UninstallDisplayIcon={app}\ttermpro.exe
AppMutex=TeraTermProAppMutex, TeraTermProMacroAppMutex, TeraTermProKeycodeAppMutex, TeraTermMenuAppMutex, CygTermAppMutex
#ifndef snapshot
OutputBaseFilename=teraterm-{#AppVer}
#else
OutputBaseFilename=teraterm-{#snapshot}
#endif
PrivilegesRequired=none

[Languages]
Name: en; MessagesFile: compiler:Default.isl
Name: ja; MessagesFile: compiler:Languages\Japanese.isl

[Dirs]
Name: {app}\Collector; Components: Collector
Name: {app}\theme; Components: TeraTerm
Name: {app}\theme\scale; Components: TeraTerm
Name: {app}\theme\tile; Components: TeraTerm
Name: {app}\plugin; Components: TeraTerm
Name: {app}\lang; Components: TeraTerm

[Files]
Source: ..\teraterm\release\ttermpro.exe; DestDir: {app}; Components: TeraTerm; Flags: ignoreversion
Source: ..\teraterm\release\ttpcmn.dll; DestDir: {app}; Components: TeraTerm
Source: ..\teraterm\release\ttpdlg.dll; DestDir: {app}; Components: TeraTerm
Source: ..\teraterm\release\ttpfile.dll; DestDir: {app}; Components: TeraTerm
Source: ..\teraterm\release\ttpset.dll; DestDir: {app}; Components: TeraTerm
Source: ..\teraterm\release\ttptek.dll; DestDir: {app}; Components: TeraTerm
Source: release\TERATERM.INI; DestDir: {app}; Components: TeraTerm; Flags: onlyifdoesntexist uninsneveruninstall; Permissions: authusers-modify
Source: release\TSPECIAL1.TTF; DestDir: {fonts}; Components: TeraTerm; Attribs: readonly; Flags: overwritereadonly uninsneveruninstall; FontInstall: Tera Special; Check: isPowerUsersMore
Source: ..\doc\en\teraterm.chm; DestDir: {app}; Components: TeraTerm
Source: ..\doc\ja\teratermj.chm; DestDir: {app}; Components: TeraTerm
Source: release\license.txt; DestDir: {app}; Components: TeraTerm
Source: release\FUNCTION.CNF; DestDir: {app}; Components: TeraTerm
Source: release\IBMKEYB.CNF; DestDir: {app}; Components: TeraTerm
Source: release\EDITOR.CNF; DestDir: {app}; Components: TeraTerm; Flags: onlyifdoesntexist uninsneveruninstall; Permissions: authusers-modify; DestName: KEYBOARD.CNF
Source: release\EDITOR.CNF; DestDir: {app}; Components: TeraTerm
Source: release\NT98KEYB.CNF; DestDir: {app}; Components: TeraTerm
Source: release\PC98KEYB.CNF; DestDir: {app}; Components: TeraTerm
Source: ..\teraterm\release\keycode.exe; DestDir: {app}; Components: TeraTerm
Source: ..\teraterm\release\ttpmacro.exe; DestDir: {app}; Components: TeraTerm; Flags: ignoreversion
Source: release\delpassw.ttl; DestDir: {app}; Components: TeraTerm
Source: release\dialup.ttl; DestDir: {app}; Components: TeraTerm
Source: release\login.ttl; DestDir: {app}; Components: TeraTerm
Source: release\mpause.ttl; DestDir: {app}; Components: TeraTerm
Source: release\random.ttl; DestDir: {app}; Components: TeraTerm
Source: release\screencapture.ttl; DestDir: {app}; Components: TeraTerm
Source: release\ssh2login.ttl; DestDir: {app}; Components: TeraTerm
Source: release\wait_regex.ttl; DestDir: {app}; Components: TeraTerm
Source: release\lang\Japanese.lng; DestDir: {app}\lang; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: release\lang\German.lng; DestDir: {app}\lang; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: ..\ttssh2\ttxssh\Release\ttxssh.dll; DestDir: {app}; Components: TTSSH; Flags: ignoreversion
Source: release\ssh_known_hosts; DestDir: {app}; Components: TTSSH; Flags: onlyifdoesntexist uninsneveruninstall; Permissions: authusers-modify
Source: ..\cygterm\cygterm.exe; DestDir: {app}; Components: cygterm
Source: ..\cygterm\cygterm.cfg; DestDir: {app}; Components: cygterm; Flags: onlyifdoesntexist uninsneveruninstall; Permissions: authusers-modify
Source: ..\cygterm\cyglaunch.exe; DestDir: {app}; Components: cygterm
Source: ..\cygterm\cygterm+.tar.gz; DestDir: {app}; Components: cygterm
Source: ..\libs\logmett\setup295.exe; DestDir: {tmp}; Components: LogMeTT; Flags: deleteafterinstall
Source: ..\ttpmenu\Release\ttpmenu.exe; DestDir: {app}; Components: TeraTerm_Menu; Flags: ignoreversion
Source: release\ttmenu_readme-j.txt; DestDir: {app}; Components: TeraTerm_Menu
Source: ..\TTProxy\Release\TTXProxy.dll; DestDir: {app}; Components: TTProxy; Flags: ignoreversion
Source: release\theme\Advanced.sample; DestDir: {app}\theme\; Components: TeraTerm
Source: release\theme\Scale.INI; DestDir: {app}\theme\; Components: TeraTerm
Source: release\theme\Tile.INI; DestDir: {app}\theme\; Components: TeraTerm
Source: release\theme\scale\23.jpg; DestDir: {app}\theme\scale; Components: TeraTerm
Source: release\theme\scale\43.jpg; DestDir: {app}\theme\scale; Components: TeraTerm
Source: release\theme\tile\03.jpg; DestDir: {app}\theme\tile; Components: TeraTerm
Source: release\theme\tile\44.jpg; DestDir: {app}\theme\tile; Components: TeraTerm
Source: release\plugin\ttAKJpeg.dll; DestDir: {app}\plugin\; Components: TeraTerm
Source: release\plugin\ttAKJpeg.txt; DestDir: {app}\plugin\; Components: TeraTerm
Source: release\Collector\Collector.exe; DestDir: {app}\Collector\; Components: Collector
Source: release\Collector\collector.ini; DestDir: {app}\Collector\; Components: Collector
Source: release\Collector\Collector_org.exe; DestDir: {app}\Collector\; Components: Collector
Source: release\Collector\hthook.dll; DestDir: {app}\Collector\; Components: Collector
Source: release\Collector\mfc70.dll; DestDir: {app}\Collector\; Components: Collector
Source: release\Collector\msvcr70.dll; DestDir: {app}\Collector\; Components: Collector
Source: release\Collector\readme.txt; DestDir: {app}\Collector\; Components: Collector
Source: ..\TTXKanjiMenu\release\ttxkanjimenu.dll; DestDir: {app}\; Components: Additional_Plugins/TTXKanjiMenu; Flags: ignoreversion
Source: ..\TTXSamples\release\TTXResizeMenu.dll; DestDir: {app}\; Components: Additional_Plugins/TTXResizeMenu; Flags: ignoreversion
Source: ..\TTXSamples\release\TTXttyrec.dll; DestDir: {app}\; Components: Additional_Plugins/TTXttyrec; Flags: ignoreversion
Source: ..\TTXSamples\release\TTXttyplay.dll; DestDir: {app}\; Components: Additional_Plugins/TTXttyrec; Flags: ignoreversion
Source: ..\TTXSamples\release\TTXKcodeChange.dll; DestDir: {app}\; Components: Additional_Plugins/TTXKcodeChange; Flags: ignoreversion
Source: ..\TTXSamples\release\TTXViewMode.dll; DestDir: {app}\; Components: Additional_Plugins/TTXViewMode; Flags: ignoreversion
Source: ..\TTXSamples\release\TTXAlwaysOnTop.dll; DestDir: {app}\; Components: Additional_Plugins/TTXAlwaysOnTop; Flags: ignoreversion
Source: ..\TTXSamples\release\TTXRecurringCommand.dll; DestDir: {app}\; Components: Additional_Plugins/TTXRecurringCommand; Flags: ignoreversion

[Types]
Name: standard; Description: {cm:type_standard}
Name: full; Description: {cm:type_full}
Name: compact; Description: {cm:type_compact}
Name: custom; Description: {cm:type_custom}; Flags: iscustom

[Components]
Name: TeraTerm; Description: Tera Term & Macro; Flags: fixed; Types: custom compact full standard
Name: TTSSH; Description: TTSSH; Types: compact full standard
Name: cygterm; Description: CygTerm+; Types: full standard; Check: not isIA64
Name: LogMeTT; Description: LogMeTT & TTLEdit; Types: full standard; MinVersion: 4.1.1998,4.0.1381sp6
Name: TeraTerm_Menu; Description: TeraTerm Menu; Types: full
Name: TTProxy; Description: TTProxy; Types: full standard
Name: Collector; Description: Collector; Types: full
Name: Additional_Plugins; Description: {cm:comp_TTX}
Name: Additional_Plugins/TTXResizeMenu; Description: TTXResizeMenu ({cm:comp_TTXResizeMenu}); Types: full standard
Name: Additional_Plugins/TTXttyrec; Description: TTXttyrec ({cm:comp_TTXttyrec}); Types: full standard
Name: Additional_Plugins/TTXKanjiMenu; Description: TTXKanjiMenu ({cm:comp_TTXKanjiMenu}); Languages: en
Name: Additional_Plugins/TTXKanjiMenu; Description: TTXKanjiMenu ({cm:comp_TTXKanjiMenu}); Types: full; Languages: ja
Name: Additional_Plugins/TTXKcodeChange; Description: TTXKcodeChange ({cm:comp_TTXKcodeChange}); Languages: en
Name: Additional_Plugins/TTXKcodeChange; Description: TTXKcodeChange ({cm:comp_TTXKcodeChange}); Types: full; Languages: ja
Name: Additional_Plugins/TTXViewMode; Description: TTXViewMode ({cm:comp_TTXViewMode}); Types: full
Name: Additional_Plugins/TTXAlwaysOnTop; Description: TTXAlwaysOnTop ({cm:comp_TTXAlwaysOnTop}); Types: full
Name: Additional_Plugins/TTXRecurringCommand; Description: TTXRecurringCommand ({cm:comp_TTXRecurringCommand}); Types: full; Languages: 

[Icons]
Name: {group}\Tera Term; Filename: {app}\ttermpro.exe; WorkingDir: {app}; IconFilename: {app}\ttermpro.exe; IconIndex: 0; Components: TeraTerm; Flags: createonlyiffileexists
Name: {group}\{cm:UninstallProgram,{#AppName}}; Filename: {uninstallexe}; Components: TeraTerm; Flags: createonlyiffileexists
Name: {group}\cyglaunch; Filename: {app}\cyglaunch.exe; WorkingDir: {app}; IconFilename: {app}\cyglaunch.exe; IconIndex: 0; Components: cygterm; Flags: createonlyiffileexists
Name: {group}\LogMeTT; Filename: {app}\LogMeTT.exe; WorkingDir: {app}; IconFilename: {app}\logMeTT.exe; IconIndex: 0; Components: LogMeTT; Flags: createonlyiffileexists
Name: {group}\TTLEdit; Filename: {app}\TTLEdit.exe; WorkingDir: {app}; IconFilename: {app}\TTLEdit.exe; IconIndex: 0; Components: LogMeTT; Flags: createonlyiffileexists
Name: {group}\TeraTerm Menu; Filename: {app}\ttpmenu.exe; WorkingDir: {app}; IconFilename: {app}\ttpmenu.exe; IconIndex: 0; Components: TeraTerm_Menu; Flags: createonlyiffileexists
Name: {group}\Collector; Filename: {app}\Collector\Collector.exe; WorkingDir: {app}\Collector; IconFilename: {app}\Collector\Collector.exe; IconIndex: 0; Components: Collector; Flags: createonlyiffileexists
Name: {userdesktop}\Tera Term; Filename: {app}\ttermpro.exe; WorkingDir: {app}; IconFilename: {app}\ttermpro.exe; Components: TeraTerm; Tasks: desktopicon; IconIndex: 0; Flags: createonlyiffileexists
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\Tera Term; Filename: {app}\ttermpro.exe; WorkingDir: {app}; IconFilename: {app}\ttermpro.exe; Components: TeraTerm; Tasks: quicklaunchicon; IconIndex: 0; Flags: createonlyiffileexists
Name: {userstartup}\TeraTerm Menu; Filename: {app}\ttpmenu.exe; WorkingDir: {app}; IconFilename: {app}\ttpmenu.exe; Components: TeraTerm_Menu; IconIndex: 0; Tasks: startupttmenuicon; Flags: createonlyiffileexists
Name: {userstartup}\Collector; Filename: {app}\collector\collector.exe; WorkingDir: {app}\Collector; IconFilename: {app}\collector\collector.exe; Components: Collector; Tasks: startupcollectoricon; IconIndex: 0; Flags: createonlyiffileexists
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\cyglaunch; Filename: {app}\cyglaunch.exe; WorkingDir: {app}; IconFilename: {app}\cyglaunch.exe; Components: cygterm; Tasks: quickcyglaunch; IconIndex: 0; Flags: createonlyiffileexists

[INI]
Filename: {app}\teraterm.ini; Section: Tera Term; Key: PasteDelayPerLine; String: 10; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: PasteDelayPerLine; String: 10; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: DisableAcceleratorDuplicateSession; String: off; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: DisableAcceleratorDuplicateSession; String: off; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: ClearScreenOnCloseConnection; String: off; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: ClearScreenOnCloseConnection; String: off; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: DisableMenuSendBreak; String: off; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: DisableMenuSendBreak; String: off; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: PasteDialogSize; String: 330,220; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: PasteDialogSize; String: 330,220; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: TTXKanjiMenu; Key: UseOneSetting; String: on; Flags: createkeyifdoesntexist; Components: Additional_Plugins/TTXKanjiMenu
Filename: {userdocs}\teraterm.ini; Section: TTXKanjiMenu; Key: UseOneSetting; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: Additional_Plugins/TTXKanjiMenu
Filename: {app}\teraterm.ini; Section: TTSSH; Key: ForwardAgent; String: 0; Flags: createkeyifdoesntexist; Components: TTSSH
Filename: {userdocs}\teraterm.ini; Section: TTSSH; Key: ForwardAgent; String: 0; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TTSSH
Filename: {app}\teraterm.ini; Section: Tera Term; Key: EnableBoldAttrColor; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: EnableBoldAttrColor; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: EnableBlinkAttrColor; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: EnableBlinkAttrColor; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: EnableReverseAttrColor; String: off; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: EnableReverseAttrColor; String: off; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: EnableURLColor; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: EnableURLColor; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: EnableANSIColor; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: EnableANSIColor; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: StrictKeyMapping; String: off; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: StrictKeyMapping; String: off; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: VTReverseColor; String: 255,255,255,0,0,0; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: VTReverseColor; String: 255,255,255,0,0,0; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: DisableWheelToCursorByCtrl; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: DisableWheelToCursorByCtrl; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: DisableMouseTrackingByCtrl; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: DisableMouseTrackingByCtrl; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: TTSSH; Key: EnableRsaShortKeyServer; String: 0; Flags: createkeyifdoesntexist; Components: TTSSH
Filename: {userdocs}\teraterm.ini; Section: TTSSH; Key: EnableRsaShortKeyServer; String: 0; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TTSSH
Filename: {app}\teraterm.ini; Section: Tera Term; Key: AcceptTitleChangeRequest; String: overwrite; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: AcceptTitleChangeRequest; String: overwrite; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: ScrollWindowClearScreen; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: ScrollWindowClearScreen; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: UnicodeToDecSpMapping; String: 3; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: UnicodeToDecSpMapping; String: 3; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: UnknownUnicodeCharacterAsWide; String: off; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: UnknownUnicodeCharacterAsWide; String: off; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: AutoScrollOnlyInBottomLine; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: AutoScrollOnlyInBottomLine; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: VTIcon; String: Default; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: VTIcon; String: Default; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: TEKIcon; String: Default; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: TEKIcon; String: Default; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: MouseWheelScrollLine; String: 3; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: MouseWheelScrollLine; String: 3; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: DisablePasteMouseMButton; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: DisablePasteMouseMButton; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: SaveVTWinPos; String: off; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: SaveVTWinPos; String: off; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: Xterm256Color; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: Xterm256Color; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: Aixterm16Color; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: Aixterm16Color; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: PcBoldColor; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: PcBoldColor; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: ConfirmChangePaste; String: off; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: ConfirmChangePaste; String: off; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: LogHideDialog; String: off; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: LogHideDialog; String: off; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: MaximizedBugTweak; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: MaximizedBugTweak; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: KillFocusCursor; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: KillFocusCursor; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: MouseEventTracking; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: MouseEventTracking; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: HostDialogOnStartup; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: HostDialogOnStartup; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: TranslateWheelToCursor; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: TranslateWheelToCursor; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: ConfirmFileDragAndDrop; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: ConfirmFileDragAndDrop; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: ZmodemRcvCommand; String: rz; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: ZmodemRcvCommand; String: rz; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: XmodemRcvCommand; String: ; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: XmodemRcvCommand; String: ; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: TelAutoDetect; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: TelAutoDetect; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: SelectOnlyByLButton; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: SelectOnlyByLButton; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: ClearComBuffOnOpen; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: ClearComBuffOnOpen; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: DisableAppCursor; String: off; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: DisableAppCursor; String: off; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: DisableAppKeypad; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: DisableAppKeypad; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: TTSSH; Key: CheckAuthListFirst; String: 0; Flags: createkeyifdoesntexist; Components: TTSSH
Filename: {userdocs}\teraterm.ini; Section: TTSSH; Key: CheckAuthListFirst; String: 0; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TTSSH
Filename: {app}\teraterm.ini; Section: Tera Term; Key: MaxBroadcatHistory; String: 99; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: MaxBroadcatHistory; String: 99; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: TelKeepAliveInterval; String: 300; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: TelKeepAliveInterval; String: 300; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: VTCompatTab; String: off; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: VTCompatTab; String: off; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: FileSendFilter; String: ; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: FileSendFilter; String: ; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: LogAutoStart; String: off; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: LogAutoStart; String: off; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: LogDefaultPath; String: ; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: LogDefaultPath; String: ; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: AcceptBroadcast; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: AcceptBroadcast; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: DisableAcceleratorSendBreak; String: off; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: DisableAcceleratorSendBreak; String: off; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: ConfirmPasteMouseRButton; String: off; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: ConfirmPasteMouseRButton; String: off; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: BroadcastCommandHistory; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: BroadcastCommandHistory; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: ConnectingTimeout; String: 0; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: ConnectingTimeout; String: 0; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: LogDefaultName; String: teraterm.log; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: LogDefaultName; String: teraterm.log; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: TTSSH; Key: RememberPassword; String: 1; Flags: createkeyifdoesntexist; Components: TTSSH
Filename: {userdocs}\teraterm.ini; Section: TTSSH; Key: RememberPassword; String: 1; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TTSSH
Filename: {app}\teraterm.ini; Section: Tera Term; Key: LogTimestamp; String: off; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: LogTimestamp; String: off; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: AlphaBlend; String: 255; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: AlphaBlend; String: 255; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: UseNormalBGColor; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: UseNormalBGColor; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: HistoryList; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: HistoryList; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: LogTypePlainText; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: LogTypePlainText; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: URLColor; String: 0,255,0,255,255,255; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: URLColor; String: 0,255,0,255,255,255; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: EnableClickableUrl; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: EnableClickableUrl; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: DisablePasteMouseRButton; String: off; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: DisablePasteMouseRButton; String: off; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: BG; Key: BGEnable; String: off; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: BG; Key: BGEnable; String: off; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: BG; Key: BGUseAlphaBlendAPI; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: BG; Key: BGUseAlphaBlendAPI; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: BG; Key: BGSPIPath; String: plugin; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: BG; Key: BGSPIPath; String: plugin; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: BG; Key: BGFastSizeMove; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: BG; Key: BGFastSizeMove; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: BG; Key: BGNoFrame; String: on; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: BG; Key: BGNoFrame; String: on; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: BG; Key: BGThemeFile; String: theme\*.ini; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: BG; Key: BGThemeFile; String: theme\*.ini; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: Tera Term; Key: ViewlogEditor; String: notepad.exe; Flags: createkeyifdoesntexist; Components: TeraTerm
Filename: {userdocs}\teraterm.ini; Section: Tera Term; Key: ViewlogEditor; String: notepad.exe; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TeraTerm
Filename: {app}\teraterm.ini; Section: TTSSH; Key: KeyboardInteractive; String: 0; Flags: createkeyifdoesntexist; Components: TTSSH
Filename: {userdocs}\teraterm.ini; Section: TTSSH; Key: KeyboardInteractive; String: 0; Flags: createkeyifdoesntexist; Check: isUserIniExists; Components: TTSSH

[Registry]
; Cygterm Here
Root: HKCU; Subkey: Software\Classes\Folder\shell\cygterm; ValueType: string; ValueData: Cy&gterm Here; Flags: uninsdeletekey; Check: isMinimumOfWin2K; Components: cygterm; Tasks: cygtermhere
Root: HKCU; Subkey: Software\Classes\Folder\shell\cygterm\command; ValueType: string; ValueData: """{app}\cyglaunch.exe"" -nocd -nols -d \""%L\"""; Flags: uninsdeletekey; Check: isMinimumOfWin2K; Components: cygterm; Tasks: cygtermhere
Root: HKCR; Subkey: Folder\shell\cygterm; ValueType: string; ValueData: Cy&gterm Here; Flags: uninsdeletekey; Check: not isMinimumOfWin2K; Components: cygterm; Tasks: cygtermhere
Root: HKCR; Subkey: Folder\shell\cygterm\command; ValueType: string; ValueData: """{app}\cyglaunch.exe"" -nocd -nols -d \""%L\"""; Flags: uninsdeletekey; Check: not isMinimumOfWin2K; Components: cygterm; Tasks: cygtermhere
; Associate with .TTL
Root: HKCU; Subkey: Software\Classes\.ttl; ValueType: string; ValueData: TeraTerm.MacroFile; Flags: uninsdeletekey; Check: isMinimumOfWin2K; Components: TeraTerm; Tasks: macroassoc
Root: HKCU; Subkey: Software\Classes\TeraTerm.MacroFile; ValueType: string; ValueData: Tera Term Macro File; Flags: uninsdeletekey; Check: isMinimumOfWin2K; Components: TeraTerm; Tasks: macroassoc
Root: HKCU; Subkey: Software\Classes\TeraTerm.MacroFile\DefaultIcon; ValueType: string; ValueData: {app}\ttpmacro.exe,0; Flags: uninsdeletekey; Check: isMinimumOfWin2K; Components: TeraTerm; Tasks: macroassoc
Root: HKCU; Subkey: Software\Classes\TeraTerm.MacroFile\shell\open\command; ValueType: string; ValueData: """{app}\ttpmacro.exe"" ""%1"""; Flags: uninsdeletekey; Check: isMinimumOfWin2K; Components: TeraTerm; Tasks: macroassoc
Root: HKCR; Subkey: .ttl; ValueType: string; ValueData: TeraTerm.MacroFile; Flags: uninsdeletekey; Check: not isMinimumOfWin2K; Components: TeraTerm; Tasks: macroassoc
Root: HKCR; Subkey: TeraTerm.MacroFile; ValueType: string; ValueData: Tera Term Macro File; Flags: uninsdeletekey; Check: not isMinimumOfWin2K; Components: TeraTerm; Tasks: macroassoc
Root: HKCR; Subkey: TeraTerm.MacroFile\DefaultIcon; ValueType: string; ValueData: {app}\ttpmacro.exe,0; Flags: uninsdeletekey; Check: not isMinimumOfWin2K; Components: TeraTerm; Tasks: macroassoc
Root: HKCR; Subkey: TeraTerm.MacroFile\shell\open\command; ValueType: string; ValueData: """{app}\ttpmacro.exe"" ""%1"""; Flags: uninsdeletekey; Check: not isMinimumOfWin2K; Components: TeraTerm; Tasks: macroassoc
; Associate with telnet://
Root: HKCU; Subkey: Software\Classes\telnet\shell; ValueType: string; ValueData: Open with Tera Term; Flags: uninsclearvalue; Check: isMinimumOfWin2K; Components: TeraTerm; Tasks: telnetassoc
Root: HKCU; Subkey: Software\Classes\telnet\shell\Open with Tera Term\command; ValueType: string; ValueData: """{app}\ttermpro.exe"" /T=1 /nossh %1"; Flags: uninsdeletekey; Check: isMinimumOfWin2K; Components: TeraTerm; Tasks: telnetassoc
Root: HKCR; Subkey: telnet\shell; ValueType: string; ValueData: Open with Tera Term; Flags: uninsclearvalue; Check: not isMinimumOfWin2K; Components: TeraTerm; Tasks: telnetassoc
Root: HKCR; Subkey: telnet\shell\Open with Tera Term\command; ValueType: string; ValueData: """{app}\ttermpro.exe"" /T=1 /nossh %1"; Flags: uninsdeletekey; Check: not isMinimumOfWin2K; Components: TeraTerm; Tasks: telnetassoc
; Associate with ssh://
Root: HKCU; Subkey: Software\Classes\ssh\shell; ValueType: string; ValueData: Open with Tera Term; Flags: uninsclearvalue; Check: isMinimumOfWin2K; Components: TTSSH; Tasks: sshassoc
Root: HKCU; Subkey: Software\Classes\ssh\shell\Open with Tera Term\command; ValueType: string; ValueData: """{app}\ttermpro.exe"" %1"; Flags: uninsdeletekey; Check: isMinimumOfWin2K; Components: TTSSH; Tasks: sshassoc
Root: HKCU; Subkey: Software\Classes\ssh; ValueName: URL Protocol; ValueType: string; Flags: uninsdeletekey; Check: isMinimumOfWin2K; Components: TTSSH; Tasks: sshassoc
Root: HKCR; Subkey: ssh\shell; ValueType: string; ValueData: Open with Tera Term; Flags: uninsclearvalue; Check: not isMinimumOfWin2K; Components: TTSSH; Tasks: sshassoc
Root: HKCR; Subkey: ssh\shell\Open with Tera Term\command; ValueType: string; ValueData: """{app}\ttermpro.exe"" %1"; Flags: uninsdeletekey; Check: not isMinimumOfWin2K; Components: TTSSH; Tasks: sshassoc
Root: HKCR; Subkey: ssh; ValueName: URL Protocol; ValueType: string; Flags: uninsdeletekey; Check: isMinimumOfWin2K; Components: TTSSH; Tasks: sshassoc
Root: HKCU; Subkey: Software\Classes\slogin\shell; ValueType: string; ValueData: Open with Tera Term; Flags: uninsclearvalue; Check: isMinimumOfWin2K; Components: TTSSH; Tasks: sshassoc
Root: HKCU; Subkey: Software\Classes\slogin\shell\Open with Tera Term\command; ValueType: string; ValueData: """{app}\ttermpro.exe"" %1"; Flags: uninsdeletekey; Check: isMinimumOfWin2K; Components: TTSSH; Tasks: sshassoc
Root: HKCU; Subkey: Software\Classes\slogin; ValueName: URL Protocol; ValueType: string; Flags: uninsdeletekey; Check: isMinimumOfWin2K; Components: TTSSH; Tasks: sshassoc
Root: HKCR; Subkey: slogin\shell; ValueType: string; ValueData: Open with Tera Term; Flags: uninsclearvalue; Check: not isMinimumOfWin2K; Components: TTSSH; Tasks: sshassoc
Root: HKCR; Subkey: slogin\shell\Open with Tera Term\command; ValueType: string; ValueData: """{app}\ttermpro.exe"" %1"; Flags: uninsdeletekey; Check: not isMinimumOfWin2K; Components: TTSSH; Tasks: sshassoc
Root: HKCR; Subkey: slogin; ValueName: URL Protocol; ValueType: string; Flags: uninsdeletekey; Check: isMinimumOfWin2K; Components: TTSSH; Tasks: sshassoc

[Tasks]
Name: desktopicon; Description: {cm:task_desktopicon}; Components: TeraTerm
Name: quicklaunchicon; Description: {cm:task_quicklaunchicon}; Components: TeraTerm
Name: startupttmenuicon; Description: {cm:task_startupttmenuicon}; Components: TeraTerm_Menu
Name: startupcollectoricon; Description: {cm:task_startupcollectoricon}; Components: Collector
Name: cygtermhere; Description: {cm:task_cygtermhere}; Components: cygterm; Flags: unchecked
Name: quickcyglaunch; Description: {cm:task_quickcyglaunch}; Components: cygterm; Flags: unchecked
Name: macroassoc; Description: {cm:task_macroassoc}; Components: TeraTerm; Flags: unchecked
Name: telnetassoc; Description: {cm:task_telnetassoc}; Components: TeraTerm; Flags: unchecked
Name: sshassoc; Description: {cm:task_sshassoc}; Components: TTSSH; Flags: unchecked

[Run]
Filename: {app}\ttermpro.exe; Flags: nowait postinstall skipifsilent unchecked; Description: {cm:launch_teraterm}; Components: TeraTerm
Filename: {tmp}\setup295.exe; Components: LogMeTT
Filename: {app}\ttpmenu.exe; Flags: nowait postinstall skipifsilent unchecked; Description: {cm:launch_ttmenu}; Components: TeraTerm_Menu
Filename: {app}\Collector\Collector.exe; Flags: nowait postinstall skipifsilent unchecked; Description: {cm:launch_collector}; Components: Collector

[UninstallRun]
Filename: {app}\uninstall.exe; Components: LogMeTT

[CustomMessages]
en.task_desktopicon=Create Tera Term shortcut to &Desktop
en.task_quicklaunchicon=Create Tera Term shortcut to &Quick Launch
en.task_startupttmenuicon=Create TeraTerm &Menu shortcut to Startup
en.task_startupcollectoricon=Create &Collector shortcut to Startup
en.task_cygtermhere=Add "Cy&gterm Here" to Context menu
en.task_quickcyglaunch=Create cyg&launch shortcut to Quick Launch
en.task_macroassoc=Associate .&ttl file to ttpmacro.exe
en.task_telnetassoc=Associate t&elnet protocol to ttermpro.exe
en.task_sshassoc=Associate &ssh protocol to ttermpro.exe
ja.task_desktopicon=デスクトップに Tera Term のショートカットを作る(&D)
ja.task_quicklaunchicon=クイック起動に Tera Term のショートカットを作る(&Q)
ja.task_startupttmenuicon=スタートアップに TeraTerm &Menu のショートカットを作る
ja.task_startupcollectoricon=スタートアップに &Collector のショートカットを作る
ja.task_cygtermhere=コンテキストメニューに "Cy&gterm Here" を追加する
ja.task_quickcyglaunch=クイック起動に cyg&launch のショートカットを作る
ja.task_macroassoc=.&ttl ファイルを ttpmacro.exe に関連付ける
ja.task_telnetassoc=t&elnet プロトコルを ttermpro.exe に関連付ける
ja.task_sshassoc=&ssh プロトコルを ttermpro.exe に関連付ける
en.type_standard=Standard installation
en.type_full=Full installation
en.type_compact=Compact installation
en.type_custom=Custom installation
ja.type_standard=標準インストール
ja.type_full=フルインストール
ja.type_compact=コンパクトインストール
ja.type_custom=カスタムインストール
en.launch_teraterm=Launch &Tera Term
en.launch_ttmenu=Launch TeraTerm &Menu
en.launch_collector=Launch &Collector
ja.launch_teraterm=今すぐ &Tera Term を実行する
ja.launch_ttmenu=今すぐ TeraTerm &Menu を実行する
ja.launch_collector=今すぐ &Collector を実行する
en.msg_language_caption=Select Language
en.msg_language_description=Which language shoud be used?
en.msg_language_subcaption=Select the language of application's menu and dialog, then click Next.
en.msg_language_none=&English
en.msg_language_japanese=&Japanese
en.msg_language_german=&German
ja.msg_language_caption=言語の選択
ja.msg_language_description=ユーザーインターフェースの言語を選択してください。
ja.msg_language_subcaption=アプリケーションのメニューやダイアログ等の表示言語を選択して、「次へ」をクリックしてください。
ja.msg_language_none=英語(&E)
ja.msg_language_japanese=日本語(&J)
ja.msg_language_german=ドイツ語(&G)
en.msg_del_confirm=Are you sure that you want to delete %s ?
ja.msg_del_confirm=%s を削除しますか？
en.msg_uninstall_confirm=It seems a former version is installed. You are recommended to uninstall it previously. Do you uninstall former version ?
ja.msg_uninstall_confirm=以前のバージョンがインストールされているようです。先にアンインストールすることをお勧めします。アンインストールしますか？
en.comp_TTX=Additional Plugins
ja.comp_TTX=追加プラグイン
en.comp_TTXResizeMenu=VT-Window size can be changed from preset
ja.comp_TTXResizeMenu=VTウィンドウのサイズをプリセット値の中から変更できるようにする
en.comp_TTXttyrec=ttyrec format record data can be recorded or playback
ja.comp_TTXttyrec=ttyrec形式の録画データを記録/再生できるようにする
en.comp_TTXKanjiMenu=Changes Japanese Kanji Code from VT-Window menu
ja.comp_TTXKanjiMenu=日本語の漢字コードをVTウィンドウのメニューから設定できるようにする
en.comp_TTXKcodeChange=Change Japanese Kanji code by remote sequence
ja.comp_TTXKcodeChange=リモートからのシーケンスで日本語の漢字コードを変更する
en.comp_TTXViewMode=View-only mode can be used
ja.comp_TTXViewMode=表示専用モードにすることができる
en.comp_TTXAlwaysOnTop=Always On Top can be used
ja.comp_TTXAlwaysOnTop=常に最前面に表示できるようにする
en.comp_TTXRecurringCommand=Recurring Command can be used
ja.comp_TTXRecurringCommand=定期的に文字列を送信する

[Code]
var
  UILangFilePage: TInputOptionWizardPage;


function isIA64 : Boolean;
begin
  if ProcessorArchitecture = paIA64 then
    Result := True
  else
    Result := False;
end;

function isMinimumOfWin2K : Boolean;
var
  Version: TWindowsVersion;
begin;
  GetWindowsVersionEx(Version);
  if Version.Major >= 5 then
    Result := True
  else
    Result := False;
end;

function isPowerUsersMore : Boolean;
begin;
  if not UsingWinNT() then begin
    Result := True;
  end else begin
    if isMinimumOfWin2K() then begin
      if IsAdminLoggedOn() or IsPowerUserLoggedOn() then begin
        Result := True;
      end else begin
        Result := False
      end;
    end else begin
      Result := True;
    end;
  end;
end;

function GetDefaultIniFilename : String;
begin
  Result := ExpandConstant('{app}') + '\TERATERM.INI';
end;

function isGetDefaultIniExists : Boolean;
var
  iniFile: String;
begin
  iniFile  := GetDefaultIniFilename();
  if FileExists(iniFile) then
    Result := True
  else
    Result := False;
end;

function GetUserIniFilename : String;
begin
  Result := ExpandConstant('{userdocs}') + '\TERATERM.INI';
end;

function isUserIniExists : Boolean;
var
  iniFile: String;
begin
  iniFile  := GetUserIniFilename();
  if FileExists(iniFile) then
    Result := True
  else
    Result := False;
end;

procedure SetIniFile(iniFile: String);
var
  Language : String;
  Locale   : String;
  CodePage : integer;
  VTFont   : String;
  TEKFont  : String;
  FileDir  : String;

begin
  Language := GetIniString('Tera Term', 'Language', '', iniFile);
  Locale   := GetIniString('Tera Term', 'Locale', '', iniFile);
  CodePage := GetIniInt('Tera Term', 'CodePage', 0, 0, 0, iniFile);
  VTFont   := GetIniString('Tera Term', 'VTFont', '', iniFile);
  TEKFont  := GetIniString('Tera Term', 'TEKFont', '', iniFile);
  FileDir  := GetIniString('Tera Term', 'FileDir', '', iniFile);

  case GetUILanguage and $3FF of
  $04: // Chinese
    begin
      if Length(Language) = 0 then
        SetIniString('Tera Term', 'Language', 'Japanese', iniFile);
      if Length(Locale) = 0 then
        SetIniString('Tera Term', 'Locale', 'chs', iniFile);
      if CodePage = 0 then
        SetIniInt('Tera Term', 'CodePage', 936, iniFile);
      if Length(VTFont) = 0 then
        SetIniString('Tera Term', 'VTFont', 'Terminal,0,-12,255', iniFile);
      if Length(TEKFont) = 0 then
        SetIniString('Tera Term', 'TEKFont', 'Terminal,0,-8,255', iniFile);
    end;
  $11: // Japanese
    begin
      if Length(Language) = 0 then
        SetIniString('Tera Term', 'Language', 'Japanese', iniFile);
      if Length(Locale) = 0 then
        SetIniString('Tera Term', 'Locale', 'japanese', iniFile);
      if CodePage = 0 then
        SetIniInt('Tera Term', 'CodePage', 932, iniFile);
      if Length(VTFont) = 0 then
        SetIniString('Tera Term', 'VTFont', 'ＭＳ 明朝,0,-16,128', iniFile);
      if Length(TEKFont) = 0 then
        SetIniString('Tera Term', 'TEKFont', 'Terminal,0,-8,128', iniFile);
    end;
  $19: // Russian
    begin
      if Length(Language) = 0 then
        SetIniString('Tera Term', 'Language', 'Russian', iniFile);
      if Length(Locale) = 0 then
        SetIniString('Tera Term', 'Locale', 'russian', iniFile);
      if CodePage = 0 then
        SetIniInt('Tera Term', 'CodePage', 1251, iniFile);
      if Length(VTFont) = 0 then
        SetIniString('Tera Term', 'VTFont', 'Terminal,0,-12,255', iniFile);
      if Length(TEKFont) = 0 then
        SetIniString('Tera Term', 'TEKFont', 'Terminal,0,-8,255', iniFile);
    end;
  else // Other
    begin

      if GetUILanguage = $409 then begin // en-US

        if Length(Language) = 0 then
          SetIniString('Tera Term', 'Language', 'Japanese', iniFile);
        if Length(Locale) = 0 then
          SetIniString('Tera Term', 'Locale', 'american', iniFile);
        if CodePage = 0 then
          SetIniInt('Tera Term', 'CodePage', 65001, iniFile);

      end else begin // Other

        if Length(Language) = 0 then
          SetIniString('Tera Term', 'Language', 'English', iniFile);
        if Length(Locale) = 0 then
          SetIniString('Tera Term', 'Locale', 'english', iniFile);
        if CodePage = 0 then
          SetIniInt('Tera Term', 'CodePage', 1252, iniFile);

      end;

      if Length(VTFont) = 0 then
        SetIniString('Tera Term', 'VTFont', 'Terminal,0,-12,255', iniFile);
      if Length(TEKFont) = 0 then
        SetIniString('Tera Term', 'TEKFont', 'Terminal,0,-8,255', iniFile);
    end;
  end;

  case UILangFilePage.SelectedValueIndex of
    1:
      SetIniString('Tera Term', 'UILanguageFile', 'lang\Japanese.lng', iniFile);
    2:
      SetIniString('Tera Term', 'UILanguageFile', 'lang\German.lng', iniFile);
    else
      SetIniString('Tera Term', 'UILanguageFile', 'lang\English.lng', iniFile);
  end;

  if Length(FileDir) = 0 then begin
    FileDir := ExpandConstant('{app}');
    SetIniString('Tera Term', 'FileDir', FileDir, iniFile);
  end;

end;

procedure InitializeWizard;
var
  UILangFilePageCaption     : String;
  UILangFilePageDescription : String;
  UILangFilePageSubCaption  : String;
  UILangFilePageNone        : String;
  UILangFilePageJapanese    : String;
  UILangFilePageGerman      : String;
begin
  UILangFilePageCaption     := CustomMessage('msg_language_caption');
  UILangFilePageDescription := CustomMessage('msg_language_description');
  UILangFilePageSubCaption  := CustomMessage('msg_language_subcaption');
  UILangFilePageNone        := CustomMessage('msg_language_none');
  UILangFilePageJapanese    := CustomMessage('msg_language_japanese');
  UILangFilePageGerman      := CustomMessage('msg_language_german');

  UILangFilePage := CreateInputOptionPage(wpSelectComponents,
    UILangFilePageCaption, UILangFilePageDescription,
    UILangFilePageSubCaption, True, False);
  UILangFilePage.Add(UILangFilePageNone);
  UILangFilePage.Add(UILangFilePageJapanese);
  UILangFilePage.Add(UILangFilePageGerman);
  case ActiveLanguage of
    'ja':
      UILangFilePage.SelectedValueIndex := 1;
    else
      UILangFilePage.SelectedValueIndex := 0;
  end;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
var
  uninstaller  : String;
  uninstaller2 : String;
  ResultCode: Integer;
  iniFile : String;
begin
  case CurPageID of

    wpWelcome:
      begin

        if RegQueryStringValue(HKEY_LOCAL_MACHINE,
                               'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\UTF-8 TeraTerm Pro with TTSSH2_is1',
                               'UninstallString', uninstaller) then
        begin
          // UTF-8 TeraTerm Pro with TTSSH2 のアンインストーラ文字列を発見した
          if not RegKeyExists(HKEY_LOCAL_MACHINE,
                              'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Tera Term_is1') then
          begin
            // Tera Term のインストールエントリが見つからない(共存していない)
            if MsgBox(CustomMessage('msg_uninstall_confirm'), mbInformation, MB_YESNO) = IDYES then
            begin
              // ユーザがアンインストールを選択した

              // 両端の " を削る
              uninstaller2 := Copy(uninstaller, 2, Length(uninstaller) - 2);

              if not Exec(uninstaller2, '', '', SW_SHOW,
                          ewWaitUntilTerminated, ResultCode) then
              begin
                // 実行に失敗
                MsgBox(SysErrorMessage(ResultCode), mbError, MB_OK);
              end;
            end;
          end;
        end;

      end;

    wpSelectComponents:
      begin

        if isUserIniExists() then
        begin
          iniFile := GetIniString('Tera Term', 'UILanguageFile', '', GetUserIniFilename());
          if iniFile = 'lang\Japanese.lng' then
            UILangFilePage.SelectedValueIndex := 1
          else if iniFile = 'lang\German.lng' then
            UILangFilePage.SelectedValueIndex := 2
          else
            UILangFilePage.SelectedValueIndex := 0;
        end if isGetDefaultIniExists() then begin
          iniFile := GetIniString('Tera Term', 'UILanguageFile', '', GetDefaultIniFilename());
          if iniFile = 'lang\Japanese.lng' then
            UILangFilePage.SelectedValueIndex := 1
          else if iniFile = 'lang\German.lng' then
            UILangFilePage.SelectedValueIndex := 2
          else
            UILangFilePage.SelectedValueIndex := 0;
        end;

      end;
  end;
  Result := True;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  iniFile : String;
begin
  case CurStep of
    ssDone:
      begin
        iniFile := GetDefaultIniFilename();
        SetIniFile(iniFile);

        if isUserIniExists() then begin
          iniFile := GetUserIniFilename();
          SetIniFile(iniFile);
        end;

        if not IsTaskSelected('cygtermhere') then
        begin;
          RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, 'Software\Classes\Folder\shell\cygterm');
          RegDeleteKeyIncludingSubkeys(HKEY_CLASSES_ROOT, 'Folder\shell\cygterm');
        end;

        if not IsTaskSelected('macroassoc') then
        begin;
          RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, 'Software\Classes\.ttl');
          RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, 'Software\Classes\TeraTerm.MacroFile');
          RegDeleteKeyIncludingSubkeys(HKEY_CLASSES_ROOT, '.ttl');
          RegDeleteKeyIncludingSubkeys(HKEY_CLASSES_ROOT, 'TeraTerm.MacroFile');
        end;

        if not IsTaskSelected('telnetassoc') then
        begin;
          RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, 'Software\Classes\telnet\shell\Open with Tera Term');
          RegDeleteValue(HKEY_CURRENT_USER, 'Software\Classes\telnet\shell', '');
          RegDeleteKeyIncludingSubkeys(HKEY_CLASSES_ROOT, 'telnet\shell\Open with Tera Term');
          RegDeleteValue(HKEY_CLASSES_ROOT, 'telnet\shell', '');
        end;

      end; // ssDone
   end; // case CurStep of
end; // CurStepChanged

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  ini     : array[0..4] of String;
  buf     : String;
  conf    : String;
  confmsg : String;
  app     : String;
  userdoc : String;
  i, res : integer;
begin
  case CurUninstallStep of
    usPostUninstall:
      begin
        ini[0] := '\TERATERM.INI';
        ini[1] := '\KEYBOARD.CNF';
        ini[2] := '\ssh_known_hosts';
        ini[3] := '\cygterm.cfg';
        ini[4] := '\broadcast.log';

        conf := CustomMessage('msg_del_confirm');
        app     := ExpandConstant('{app}');
        userdoc := ExpandConstant('{userdocs}');

        // delete config files
        for i := 0 to 4 do
        begin
          buf := app + ini[i];
          if FileExists(buf) then begin
            confmsg := Format(conf, [buf]);
            res := MsgBox(confmsg, mbInformation, MB_YESNO or MB_DEFBUTTON2);
            if res = IDYES then
              DeleteFile(buf);
          end;
          buf := userdoc + ini[i];
          if FileExists(buf) then begin
            confmsg := Format(conf, [buf]);
            res := MsgBox(confmsg, mbInformation, MB_YESNO or MB_DEFBUTTON2);
            if res = IDYES then
              DeleteFile(buf);
          end;
        end;

        // delete registory
        if RegKeyExists(HKEY_CURRENT_USER, 'Software\ShinpeiTools\TTermMenu') then begin
          confmsg := Format(conf, ['HKEY_CURRENT_USER' + '\Software\ShinpeiTools\TTermMenu']);
          res := MsgBox(confmsg, mbInformation, MB_YESNO or MB_DEFBUTTON2);
          if res = IDYES then begin
            RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, 'Software\ShinpeiTools\TTermMenu');
            RegDeleteKeyIfEmpty(HKEY_CURRENT_USER, 'Software\ShinpeiTools');
          end;
        end;

        // directory is deleted only if empty
        RemoveDir(app);
      end;
  end;
end;

[InstallDelete]
Name: {app}\OpenSSH-LICENCE.txt; Type: files
Name: {app}\cygterm-README.txt; Type: files
Name: {app}\cygterm-README-j.txt; Type: files
Name: {app}\keycode.txt; Type: files
Name: {app}\keycodej.txt; Type: files
Name: {app}\RE.txt; Type: files
Name: {app}\RE-ja.txt; Type: files
Name: {app}\ssh2_readme.txt; Type: files
Name: {app}\ssh2_readme-j.txt; Type: files
Name: {app}\utf8_readme.txt; Type: files
Name: {app}\utf8_readme-j.txt; Type: files
Name: {app}\OpenSSH-LICENSE.txt; Type: files
Name: {app}\OpenSSL-LICENSE.txt; Type: files
Name: {group}\TeraTerm Document.lnk; Type: files
Name: {group}\TeraTerm Document(Japanese).lnk; Type: files
Name: {group}\TTSSH Document.lnk; Type: files
Name: {group}\TTSSH Document(Japanese).lnk; Type: files
Name: {app}\LogMeTT.hlp; Type: files
Name: {app}\macro.hlp; Type: files
Name: {app}\macroj.hlp; Type: files
Name: {app}\ttermp.hlp; Type: files
Name: {app}\ttermpj.hlp; Type: files
Name: {app}\copyfont.bat; Type: files
Name: {app}\copyfont.pif; Type: files
Name: {app}\libeay.txt; Type: files

[_ISToolPreCompile]
Name: makechm.bat
; Name: build.bat; Parameters: rebuild
Name: build.bat
