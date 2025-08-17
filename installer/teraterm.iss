﻿#define AppName "Tera Term"

; 出力ファイル名(exeなし)
#ifndef OutputBaseFilename
#define OutputBaseFilename "teraterm-wild_build"
#endif

; App Version
#ifndef AppVersion
#define AppVersion "wild_build"
#endif

; source dir
#ifndef SrcDir
#define SrcDir "teraterm"
#endif

[InnoIDE_PreCompile]
Name: makechm.bat
Name: build.bat
;Name: build.bat; Parameters: rebuild

[InnoIDE_PostCompile]
;Name: makearchive.bat; Parameters: release

[PreCompile]
Name: makechm.bat
Name: build.bat
;Name: build.bat; Parameters: rebuild

[PostCompile]
;Name: makearchive.bat; Parameters: release

[_ISToolPreCompile]
Name: makechm.bat
Name: build.bat
;Name: build.bat; Parameters: rebuild

[_ISToolPostCompile]
;Name: makearchive.bat; Parameters: release

[Setup]
AppName={#AppName}
AppId={{07A7E17A-F6D6-44A7-82E6-6BEE528CCA2A}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}

; properties of installer executable
VersionInfoDescription={#AppName} installer
AppCopyright=(C) 2004-2025 TeraTerm Project
; Apps in Settings
AppPublisher=TeraTerm Project
AppPublisherURL=https://teratermproject.github.io/
AppSupportURL=https://github.com/TeraTermProject/teraterm/issues
; execute
PrivilegesRequired=none
; during installer execution
ShowLanguageDialog=yes
UsePreviousLanguage=no
LicenseFile={#SrcDir}\license.txt
DefaultDirName={commonpf}\teraterm5
AllowNoIcons=true
DefaultGroupName={#AppName} 5
; uninstall
UninstallDisplayIcon={app}\ttermpro.exe
; create installer exe
OutputBaseFilename={#OutputBaseFilename}
SolidCompression=yes
Compression=lzma2/ultra64
#if defined(M_X64)
ArchitecturesAllowed=win64
ArchitecturesInstallIn64BitMode=x64os
#elif defined(M_ARM64)
ArchitecturesAllowed=arm64
ArchitecturesInstallIn64BitMode=arm64
#else
ArchitecturesAllowed=x86compatible
ArchitecturesInstallIn64BitMode=
#endif

[Languages]
Name: en; MessagesFile: compiler:Default.isl,message_en_US.isl
Name: ja; MessagesFile: compiler:Languages\Japanese.isl,message_en_US.isl,message_ja_JP.isl
Name: it; MessagesFile: compiler:Languages\Italian.isl,message_en_US.isl,message_it_IT.isl
Name: tr; MessagesFile: compiler:Languages\Turkish.isl,message_tr_TR.isl,message_tr_TR.isl

[Dirs]
Name: {app}\theme; Components: TeraTerm
Name: {app}\theme\scale; Components: TeraTerm
Name: {app}\theme\tile; Components: TeraTerm
Name: {app}\plugin; Components: TeraTerm
Name: {app}\lang; Components: TeraTerm
Name: {app}\lang_utf16le; Components: TeraTerm

[Files]
Source: {#SrcDir}\ttermpro.exe; DestDir: {app}; Components: TeraTerm; Flags: ignoreversion
Source: {#SrcDir}\ttpcmn.dll; DestDir: {app}; Components: TeraTerm; Flags: ignoreversion
Source: {#SrcDir}\TERATERM.INI; DestDir: {app}; Components: TeraTerm
Source: {#SrcDir}\TSPECIAL1.TTF; DestDir: {commonfonts}; Components: TeraTerm; Attribs: readonly; Flags: onlyifdoesntexist overwritereadonly uninsneveruninstall; FontInstall: Tera Special; Check: isAbleToInstallFont
;Source: {#SrcDir}\TSPECIAL1.TTF; DestDir: {app}; Components: TeraTerm
Source: {#SrcDir}\teraterm.chm; DestDir: {app}; Components: TeraTerm
Source: {#SrcDir}\teratermj.chm; DestDir: {app}; Components: TeraTerm
Source: {#SrcDir}\license.txt; DestDir: {app}; Components: TeraTerm
Source: {#SrcDir}\IBMKEYB.CNF; DestDir: {app}; Components: TeraTerm
Source: {#SrcDir}\IBMKEYB.CNF; DestDir: {app}; Components: TeraTerm; DestName: KEYBOARD.CNF
Source: {#SrcDir}\VT200.CNF; DestDir: {app}; Components: TeraTerm
Source: {#SrcDir}\keycode.exe; DestDir: {app}; Components: TeraTerm; Flags: ignoreversion
Source: {#SrcDir}\ttpmacro.exe; DestDir: {app}; Components: TeraTerm; Flags: ignoreversion
Source: {#SrcDir}\delpassw.ttl; DestDir: {app}; Components: TeraTerm
Source: {#SrcDir}\dialup.ttl; DestDir: {app}; Components: TeraTerm
Source: {#SrcDir}\login.ttl; DestDir: {app}; Components: TeraTerm
Source: {#SrcDir}\mpause.ttl; DestDir: {app}; Components: TeraTerm
Source: {#SrcDir}\random.ttl; DestDir: {app}; Components: TeraTerm
Source: {#SrcDir}\screencapture.ttl; DestDir: {app}; Components: TeraTerm
Source: {#SrcDir}\ssh2login.ttl; DestDir: {app}; Components: TeraTerm
Source: {#SrcDir}\wait_regex.ttl; DestDir: {app}; Components: TeraTerm
Source: {#SrcDir}\lang\Default.lng; DestDir: {app}\lang; Components: TeraTerm; Flags: onlyifdoesntexist uninsneveruninstall; Permissions: authusers-modify
Source: {#SrcDir}\lang\ja_JP.lng; DestDir: {app}\lang; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang\de_DE.lng; DestDir: {app}\lang; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang\fr_FR.lng; DestDir: {app}\lang; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang\ru_RU.lng; DestDir: {app}\lang; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang\ko_KR.lng; DestDir: {app}\lang; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang\zh_CN.lng; DestDir: {app}\lang; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang\es_ES.lng; DestDir: {app}\lang; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang\zh_TW.lng; DestDir: {app}\lang; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang\ta_IN.lng; DestDir: {app}\lang; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang\pt_BR.lng; DestDir: {app}\lang; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang\it_IT.lng; DestDir: {app}\lang; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang\tr_TR.lng; DestDir: {app}\lang; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang_utf16le\Default.lng; DestDir: {app}\lang_utf16le; Components: TeraTerm; Flags: onlyifdoesntexist uninsneveruninstall; Permissions: authusers-modify
Source: {#SrcDir}\lang_utf16le\ja_JP.lng; DestDir: {app}\lang_utf16le; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang_utf16le\de_DE.lng; DestDir: {app}\lang_utf16le; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang_utf16le\fr_FR.lng; DestDir: {app}\lang_utf16le; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang_utf16le\ru_RU.lng; DestDir: {app}\lang_utf16le; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang_utf16le\ko_KR.lng; DestDir: {app}\lang_utf16le; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang_utf16le\zh_CN.lng; DestDir: {app}\lang_utf16le; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang_utf16le\es_ES.lng; DestDir: {app}\lang_utf16le; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang_utf16le\zh_TW.lng; DestDir: {app}\lang_utf16le; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang_utf16le\ta_IN.lng; DestDir: {app}\lang_utf16le; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang_utf16le\pt_BR.lng; DestDir: {app}\lang_utf16le; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang_utf16le\it_IT.lng; DestDir: {app}\lang_utf16le; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\lang_utf16le\tr_TR.lng; DestDir: {app}\lang_utf16le; Components: TeraTerm; Attribs: readonly; Flags: uninsremovereadonly overwritereadonly
Source: {#SrcDir}\ttxssh.dll; DestDir: {app}; Components: TTSSH; Flags: ignoreversion
Source: {#SrcDir}\ssh_known_hosts; DestDir: {app}; Components: TTSSH
Source: {#SrcDir}\cygterm.cfg; DestDir: {app}; Components: cygterm
Source: {#SrcDir}\cygterm+.tar.gz; DestDir: {app}; Components: cygterm
Source: {#SrcDir}\cygterm.exe; DestDir: {app}; Components: cygterm
Source: {#SrcDir}\cyglaunch.exe; DestDir: {app}; Components: cygterm
Source: {#SrcDir}\ttpmenu.exe; DestDir: {app}; Components: TeraTerm_Menu; Flags: ignoreversion
Source: {#SrcDir}\ttmenu_readme-j.txt; DestDir: {app}; DestName: "ttmenu_readme-j.txt"; Components: TeraTerm_Menu
Source: {#SrcDir}\TTXProxy.dll; DestDir: {app}; Components: TTProxy; Flags: ignoreversion
Source: {#SrcDir}\theme\Advanced.sample; DestDir: {app}\theme\; Components: TeraTerm
Source: {#SrcDir}\theme\ImageFile.INI; DestDir: {app}\theme\; Components: TeraTerm
Source: {#SrcDir}\theme\Scale.INI; DestDir: {app}\theme\; Components: TeraTerm
Source: {#SrcDir}\theme\Tile.INI; DestDir: {app}\theme\; Components: TeraTerm
Source: {#SrcDir}\theme\scale\23.jpg; DestDir: {app}\theme\scale; Components: TeraTerm
Source: {#SrcDir}\theme\scale\43.jpg; DestDir: {app}\theme\scale; Components: TeraTerm
Source: {#SrcDir}\theme\tile\03.jpg; DestDir: {app}\theme\tile; Components: TeraTerm
Source: {#SrcDir}\theme\tile\44.jpg; DestDir: {app}\theme\tile; Components: TeraTerm
Source: {#SrcDir}\ttxkanjimenu.dll; DestDir: {app}\; Components: Additional_Plugins/TTXKanjiMenu; Flags: ignoreversion
Source: {#SrcDir}\TTXResizeMenu.dll; DestDir: {app}\; Components: Additional_Plugins/TTXResizeMenu; Flags: ignoreversion
Source: {#SrcDir}\TTXttyrec.dll; DestDir: {app}\; Components: Additional_Plugins/TTXttyrec; Flags: ignoreversion
Source: {#SrcDir}\TTXttyplay.dll; DestDir: {app}\; Components: Additional_Plugins/TTXttyrec; Flags: ignoreversion
Source: {#SrcDir}\TTXKcodeChange.dll; DestDir: {app}\; Components: Additional_Plugins/TTXKcodeChange; Flags: ignoreversion
Source: {#SrcDir}\TTXViewMode.dll; DestDir: {app}\; Components: Additional_Plugins/TTXViewMode; Flags: ignoreversion
Source: {#SrcDir}\TTXAlwaysOnTop.dll; DestDir: {app}\; Components: Additional_Plugins/TTXAlwaysOnTop; Flags: ignoreversion
Source: {#SrcDir}\TTXRecurringCommand.dll; DestDir: {app}\; Components: Additional_Plugins/TTXRecurringCommand; Flags: ignoreversion

[Types]
Name: standard; Description: {cm:type_standard}
Name: full; Description: {cm:type_full}
Name: compact; Description: {cm:type_compact}
Name: custom; Description: {cm:type_custom}; Flags: iscustom

[Components]
Name: TeraTerm; Description: Tera Term & Macro; Flags: fixed; Types: custom compact full standard
Name: TTSSH; Description: TTSSH; Types: compact full standard
Name: cygterm; Description: CygTerm+; Types: full standard; Check: isExecutableCygtermX64
Name: TeraTerm_Menu; Description: TeraTerm Menu; Types: full
Name: TTProxy; Description: TTProxy; Types: full standard
Name: Additional_Plugins; Description: {cm:comp_TTX}
Name: Additional_Plugins/TTXResizeMenu; Description: TTXResizeMenu ({cm:comp_TTXResizeMenu}); Types: full standard
Name: Additional_Plugins/TTXttyrec; Description: TTXttyrec ({cm:comp_TTXttyrec}); Types: full standard
Name: Additional_Plugins/TTXKanjiMenu; Description: TTXKanjiMenu ({cm:comp_TTXKanjiMenu}); Languages: en
Name: Additional_Plugins/TTXKanjiMenu; Description: TTXKanjiMenu ({cm:comp_TTXKanjiMenu}); Types: full; Languages: ja
Name: Additional_Plugins/TTXKcodeChange; Description: TTXKcodeChange ({cm:comp_TTXKcodeChange}); Languages: en
Name: Additional_Plugins/TTXKcodeChange; Description: TTXKcodeChange ({cm:comp_TTXKcodeChange}); Types: full; Languages: ja
Name: Additional_Plugins/TTXViewMode; Description: TTXViewMode ({cm:comp_TTXViewMode}); Types: full
Name: Additional_Plugins/TTXAlwaysOnTop; Description: TTXAlwaysOnTop ({cm:comp_TTXAlwaysOnTop}); Types: full
Name: Additional_Plugins/TTXRecurringCommand; Description: TTXRecurringCommand ({cm:comp_TTXRecurringCommand}); Types: full

[Icons]
Name: {group}\Tera Term; Filename: {app}\ttermpro.exe; WorkingDir: {app}; IconFilename: {app}\ttermpro.exe; IconIndex: 0; Components: TeraTerm; Flags: createonlyiffileexists
Name: {group}\{cm:UninstallProgram,{#AppName}}; Filename: {uninstallexe}; Components: TeraTerm; Flags: createonlyiffileexists
Name: {group}\cyglaunch; Filename: {app}\cyglaunch.exe; WorkingDir: {app}; IconFilename: {app}\cyglaunch.exe; IconIndex: 0; Components: cygterm; Flags: createonlyiffileexists
Name: {group}\TeraTerm Menu; Filename: {app}\ttpmenu.exe; WorkingDir: {app}; IconFilename: {app}\ttpmenu.exe; IconIndex: 0; Components: TeraTerm_Menu; Flags: createonlyiffileexists
Name: {userdesktop}\Tera Term 5; Filename: {app}\ttermpro.exe; WorkingDir: {app}; IconFilename: {app}\ttermpro.exe; Components: TeraTerm; Tasks: desktopicon; IconIndex: 0; Flags: createonlyiffileexists
Name: {userstartup}\TeraTerm Menu; Filename: {app}\ttpmenu.exe; WorkingDir: {app}; IconFilename: {app}\ttpmenu.exe; Components: TeraTerm_Menu; IconIndex: 0; Tasks: startupttmenuicon; Flags: createonlyiffileexists

[Registry]
; Register ProgId
;   アンインストール時にはキーごと削除する。
Root: HKLM; Subkey: Software\Classes\TeraTerm.telnet; ValueType: string; ValueData: "URL: TELNET Protocol"; Flags: uninsdeletekey; Components: TeraTerm; Tasks: telnetassoc
Root: HKLM; Subkey: Software\Classes\TeraTerm.telnet; ValueName: EditFlags; ValueType: dword; ValueData: 2; Components: TeraTerm; Tasks: telnetassoc
Root: HKLM; Subkey: Software\Classes\TeraTerm.telnet; ValueName: URL Protocol; ValueType: string; Components: TeraTerm; Tasks: telnetassoc
Root: HKLM; Subkey: Software\Classes\TeraTerm.telnet\DefaultIcon; ValueType: string; ValueData: """{app}\ttermpro.exe"",0"; Components: TeraTerm; Tasks: telnetassoc
Root: HKLM; Subkey: Software\Classes\TeraTerm.telnet\shell\open\command; ValueType: string; ValueData: """{app}\ttermpro.exe"" /T=1 /nossh /E %1"; Components: TeraTerm; Tasks: telnetassoc
Root: HKLM; Subkey: Software\Classes\TeraTerm.ssh; ValueType: string; ValueData: "URL: SSH Protocol"; Flags: uninsdeletekey; Components: TTSSH; Tasks: sshassoc
Root: HKLM; Subkey: Software\Classes\TeraTerm.ssh; ValueName: EditFlags; ValueType: dword; ValueData: 2; Components: TTSSH; Tasks: sshassoc
Root: HKLM; Subkey: Software\Classes\TeraTerm.ssh; ValueName: URL Protocol; ValueType: string; Components: TTSSH; Tasks: telnetassoc
Root: HKLM; Subkey: Software\Classes\TeraTerm.ssh\DefaultIcon; ValueType: string; ValueData: """{app}\ttxssh.dll"",0"; Components: TTSSH; Tasks: sshassoc
Root: HKLM; Subkey: Software\Classes\TeraTerm.ssh\shell\open\command; ValueType: string; ValueData: """{app}\ttermpro.exe"" /ssh %1"; Components: TTSSH; Tasks: sshassoc
Root: HKLM; Subkey: Software\Classes\TeraTerm.MacroFile; ValueType: string; ValueData: "Tera Term Macro File"; Flags: uninsdeletekey; Components: TeraTerm; Tasks: macroassoc
Root: HKLM; Subkey: Software\Classes\TeraTerm.MacroFile\DefaultIcon; ValueType: string; ValueData: """{app}\ttpmacro.exe"",3"; Components: TeraTerm; Tasks: macroassoc
Root: HKLM; Subkey: Software\Classes\TeraTerm.MacroFile\shell\open\command; ValueType: string; ValueData: """{app}\ttpmacro.exe"" ""%1"""; Components: TeraTerm; Tasks: macroassoc
Root: HKLM; Subkey: Software\Classes\TeraTerm.TTYRecordFile; ValueType: string; ValueData: "Tera Term TTY Record File"; Flags: uninsdeletekey; Components: Additional_Plugins/TTXttyrec; Tasks: ttyplayassoc
Root: HKLM; Subkey: Software\Classes\TeraTerm.TTYRecordFile\DefaultIcon; ValueType: string; ValueData: """{app}\ttermpro.exe"",0"; Components: Additional_Plugins/TTXttyrec; Tasks: ttyplayassoc
Root: HKLM; Subkey: Software\Classes\TeraTerm.TTYRecordFile\shell\open\command; ValueType: string; ValueData: """{app}\ttermpro.exe"" /R=""%1"" /TTYPLAY"; Components: Additional_Plugins/TTXttyrec; Tasks: ttyplayassoc

; Register Application
;   Software\RegisteredApplications MUST NOT uninsdeletekey
Root: HKLM; Subkey: Software\RegisteredApplications; ValueType: string; ValueName: "Tera Term"; ValueData: "Software\Tera Term\Capabilities"; Flags: uninsdeletevalue; Components: TeraTerm; Tasks: telnetassoc sshassoc
;   アンインストール時にはキーごと削除する。
Root: HKLM; Subkey: Software\Tera Term; Flags: uninsdeletekey; Components: TeraTerm; Tasks: telnetassoc sshassoc
Root: HKLM; Subkey: Software\Tera Term\Capabilities; ValueType: string; ValueName: "ApplicationDescription"; ValueData: "Tera Term"; Components: TeraTerm; Tasks: telnetassoc sshassoc

; Associate ProgId to Protocol
;   プロトコル と 選択できるプログラム の関連付け
Root: HKLM; Subkey: Software\Tera Term\Capabilities\UrlAssociations; ValueType: string; ValueName: "telnet"; ValueData: "TeraTerm.telnet"; Components: TeraTerm; Tasks: telnetassoc
Root: HKLM; Subkey: Software\Tera Term\Capabilities\UrlAssociations; ValueType: string; ValueName: "ssh"; ValueData: "TeraTerm.ssh"; Components: TTSSH; Tasks: sshassoc
Root: HKLM; Subkey: Software\Tera Term\Capabilities\UrlAssociations; ValueType: string; ValueName: "slogin"; ValueData: "TeraTerm.ssh"; Components: TTSSH; Tasks: sshassoc

; Legacy Association
;   プロトコルにプログラムを直接指定する XP 以前の登録の仕方だが、
;   これがないと プロトコル/リンクの種類 の関連付け画面に プロトコル が出てこない。
;   HKCR に登録してもプロトコルは出てこない。
;   telnet は Windows に元々あるので追加しなくてよい。
;   アンインストール時にはプロトコルだけ残す。
Root: HKLM; Subkey: Software\Classes\ssh; ValueType: string; ValueData: "URL: SSH Protocol"; Components: TTSSH; Tasks: sshassoc
Root: HKLM; Subkey: Software\Classes\ssh; ValueName: URL Protocol; ValueType: string; Components: TTSSH; Tasks: sshassoc
Root: HKLM; Subkey: Software\Classes\ssh; ValueName: EditFlags; ValueType: dword; ValueData: 2; Components: TTSSH; Tasks: sshassoc
Root: HKLM; Subkey: Software\Classes\ssh\DefaultIcon; ValueType: string; ValueData: """{app}\ttxssh.dll"",0"; Flags: uninsdeletekey; Components: TTSSH; Tasks: sshassoc
Root: HKLM; Subkey: Software\Classes\ssh\shell\open\command; ValueType: string; ValueData: """{app}\ttermpro.exe"" /ssh %1"; Flags: uninsdeletekey; Components: TTSSH; Tasks: sshassoc
Root: HKLM; Subkey: Software\Classes\slogin; ValueType: string; ValueData: "URL: slogin Protocol"; Components: TTSSH; Tasks: sshassoc
Root: HKLM; Subkey: Software\Classes\slogin; ValueName: URL Protocol; ValueType: string; Components: TTSSH; Tasks: sshassoc
Root: HKLM; Subkey: Software\Classes\slogin; ValueName: EditFlags; ValueType: dword; ValueData: 2; Components: TTSSH; Tasks: sshassoc
Root: HKLM; Subkey: Software\Classes\slogin\DefaultIcon; ValueType: string; ValueData: """{app}\ttxssh.dll"",0"; Flags: uninsdeletekey; Components: TTSSH; Tasks: sshassoc
Root: HKLM; Subkey: Software\Classes\slogin\shell\open\command; ValueType: string; ValueData: """{app}\ttermpro.exe"" /ssh %1"; Flags: uninsdeletekey; Components: TTSSH; Tasks: sshassoc
;   拡張子 と プログラム の関連付け
;   この拡張子がほかのプログラムとバッティングすることはないと思われるので、RegisteredApplications への登録はしない。
;   Inno Setup のサンプルでも Classes\.ext に直接書き込んでいる（Examples\Example3.iss）。
;   アンインストール時 .ext は残す。
Root: HKLM; Subkey: Software\Classes\.ttl\OpenWithProgids; ValueType: string; ValueName: "TeraTerm.MacroFile"; Flags: uninsdeletevalue; Components: TeraTerm; Tasks: macroassoc
Root: HKLM; Subkey: Software\Classes\.tty\OpenWithProgids; ValueType: string; ValueName: "TeraTerm.TTYRecordFile"; Flags: uninsdeletevalue; Components: Additional_Plugins/TTXttyrec; Tasks: ttyplayassoc

; Cygterm Here
Root: HKCU; Subkey: Software\Classes\Folder\shell\cygterm; Flags: uninsdeletekey; Components: cygterm; Tasks: cygtermhere
Root: HKCU; Subkey: Software\Classes\Folder\shell\cygterm; ValueType: string; ValueData: Cy&gterm Here; Components: cygterm; Tasks: cygtermhere
Root: HKCU; Subkey: Software\Classes\Folder\shell\cygterm; ValueType: string; ValueName: Icon; ValueData: """{app}\cyglaunch.exe"""; Components: cygterm; Tasks: cygtermhere
Root: HKCU; Subkey: Software\Classes\Folder\shell\cygterm\command; ValueType: string; ValueData: """{app}\cyglaunch.exe"" -nocd -v CHERE_INVOKING=y -d ""\""%L\"""""; Components: cygterm; Tasks: cygtermhere
; Cygterm Here from folder Background
Root: HKCU; Subkey: Software\Classes\Directory\Background\shell\cygterm; Flags: uninsdeletekey; Components: cygterm; Tasks: cygtermhere
Root: HKCU; Subkey: Software\Classes\Directory\Background\shell\cygterm; ValueType: string; ValueData: Cy&gterm Here; Components: cygterm; Tasks: cygtermhere
Root: HKCU; Subkey: Software\Classes\Directory\Background\shell\cygterm; ValueType: string; ValueName: Icon; ValueData: """{app}\cyglaunch.exe"""; Components: cygterm; Tasks: cygtermhere
Root: HKCU; Subkey: Software\Classes\Directory\Background\shell\cygterm\command; ValueType: string; ValueData: """{app}\cyglaunch.exe"" -nocd -v CHERE_INVOKING=y -d ""\""%V\"""""; Components: cygterm; Tasks: cygtermhere
Root: HKCU; Subkey: Software\Classes\LibraryFolder\Background\shell\cygterm; Flags: uninsdeletekey; Components: cygterm; Tasks: cygtermhere
Root: HKCU; Subkey: Software\Classes\LibraryFolder\Background\shell\cygterm; ValueType: string; ValueData: Cy&gterm Here; Components: cygterm; Tasks: cygtermhere
Root: HKCU; Subkey: Software\Classes\LibraryFolder\Background\shell\cygterm; ValueType: string; ValueName: Icon; ValueData: """{app}\cyglaunch.exe"""; Components: cygterm; Tasks: cygtermhere
Root: HKCU; Subkey: Software\Classes\LibraryFolder\Background\shell\cygterm\command; ValueType: string; ValueData: """{app}\cyglaunch.exe"" -nocd -v CHERE_INVOKING=y -d ""\""%V\"""""; Components: cygterm; Tasks: cygtermhere

[Tasks]
Name: desktopicon; Description: {cm:task_desktopicon}; Components: TeraTerm
Name: startupttmenuicon; Description: {cm:task_startupttmenuicon}; Components: TeraTerm_Menu
Name: cygtermhere; Description: {cm:task_cygtermhere}; Components: cygterm; Flags: unchecked
Name: macroassoc; Description: {cm:task_macroassoc}; Components: TeraTerm; Flags: unchecked
Name: telnetassoc; Description: {cm:task_telnetassoc}; Components: TeraTerm; Flags: unchecked
Name: sshassoc; Description: {cm:task_sshassoc}; Components: TTSSH; Flags: unchecked
Name: ttyplayassoc; Description: {cm:task_ttyplayassoc}; Components: Additional_Plugins/TTXttyrec; Flags: unchecked

[Run]
Filename: {app}\ttermpro.exe; Flags: nowait postinstall skipifsilent unchecked; Description: {cm:launch_teraterm}; Components: TeraTerm
Filename: {app}\ttpmenu.exe; Flags: nowait postinstall skipifsilent unchecked; Description: {cm:launch_ttmenu}; Components: TeraTerm_Menu

[Code]
const
  SHCNF_IDLIST = $0000;
  SHCNE_ASSOCCHANGED = $08000000;
  IMAGE_FILE_MACHINE_UNKNOWN = $0000;
  IMAGE_FILE_MACHINE_I386 = $014c;
  IMAGE_FILE_MACHINE_AMD64 = $8664;

procedure SHChangeNotify(wEventId, uFlags, dwItem1, dwItem2: Integer);
external 'SHChangeNotify@shell32.dll stdcall';

var
  UILangFilePage: TInputOptionWizardPage;


// Windows 11 or later
function isWin11OrLater : Boolean;
var
  Version: TWindowsVersion;
begin;
  GetWindowsVersionEx(Version);
  if (Version.Major >= 10) and (Version.Build >= 22000) then
    Result := True
  else
    Result := False;
end;

// Cygterm x86_64 is executable
function isExecutableCygtermX64 : Boolean;
begin
  if ProcessorArchitecture = paX64 then
    Result := True
  else if ProcessorArchitecture = paARM64 then
    // x86_64 binary is executable on ARM64 by WoW64
    if isWin11OrLater then
      Result := True
    else
      Result := False
  else
    Result := False;
end;

// Admins or PowerUsers
function isAbleToInstallFont : Boolean;
begin;
  if IsAdmin() then begin
    Result := True;
  end else begin
    Result := False
  end;
end;

{
// If value is not found in INI, returns ""
function GetIniString2(Section:String; Key:String; Filename:String) : String;
var
  Default1: String;
  Default2: String;
  Value1:   String;
  Value2:   String;
begin
  Default1 := 'on';
  Default2 := 'off';
  Value1 := GetIniString(Section, Key, Default1, Filename);
  Value2 := GetIniString(Section, Key, Default2, Filename);

  if Value1 <> Value2 then
    // no value in INI
    Result := ''
  else
    // value in INI
    Result := Value1
end;
}

function CheckFileUsing(Filename:String) : integer;
var
  TmpFileName : String;
begin
  if FileExists(FileName) then
    begin
      TmpFileName := FileName + '.' + GetDateTimeString('yyyymmddhhnnss', #0, #0); // Tmp file ends with timestamp
      if CopyFile(FileName, TmpFileName, True) then
        if DeleteFile(FileName) then
          if RenameFile(TmpFileName, FileName) then
            Result := 0
          else
            Result := -1 // permission?
        else
          begin
            Result := 1; // failed to delete
            DeleteFile(TmpFileName);
          end
      else
        Result := -1 // permission?
    end
  else
    Result := 0;
end;

function CheckAppsUsing() : string;
var
  FileDir  : String;
  FileName : array[0..6] of String;
  FileDesc : array[0..6] of String;
  i        : integer;
begin
  FileDir := ExpandConstant('{app}');
  FileName[0] := FileDir + '\ttermpro.exe';
  FileName[1] := FileDir + '\ttpmacro.exe';
  FileName[2] := FileDir + '\keycode.exe';
  FileName[3] := FileDir + '\ttpmenu.exe';
  FileName[4] := FileDir + '\cygterm.exe';
  FileDesc[0] := 'Tera Term';
  FileDesc[1] := 'Tera Term Macro';
  FileDesc[2] := 'Keycode';
  FileDesc[3] := 'TeraTerm Menu';
  FileDesc[4] := 'CygTerm+';

  for i := 0 to 4 do
  begin
    case CheckFileUsing(FileName[i]) of
      1:
        // Failed to delete. In use.
        begin
          if Length(Result) > 0 then
            Result := Result + ', ' + FileDesc[i]
          else
            Result := FileDesc[i]
        end;
      else
        // -1: Failed to copy/rename
        //  0: OK
        // NOP
    end;
  end;

end;

function GetDefaultIniFilename : String;
begin
  Result := ExpandConstant('{app}') + '\TERATERM.INI';
end;

procedure SetIniFile(iniFile: String);
var
  Language      : String;
  VTFont        : String;
  TEKFont       : String;
  TCPPort       : integer;
  ViewlogEditor : String;
  CipherOrder   : String;

begin
  Language       := GetIniString('Tera Term', 'Language', '', iniFile);
  VTFont         := GetIniString('Tera Term', 'VTFont', '', iniFile);
  TEKFont        := GetIniString('Tera Term', 'TEKFont', '', iniFile);
  TCPPort        := GetIniInt('Tera Term', 'TCPPort', 0, 0, 65535, iniFile)
  ViewlogEditor  := GetIniString('Tera Term', 'ViewlogEditor', '', iniFile);
  CipherOrder    := GetIniString('TTSSH', 'CipherOrder', '', iniFile);

  case GetUILanguage and $3FF of
  $04: // Chinese
    begin
      if Length(Language) = 0 then
        SetIniString('Tera Term', 'Language', 'UTF-8', iniFile);
      if Length(VTFont) = 0 then
        SetIniString('Tera Term', 'VTFont', 'Terminal,0,-12,255', iniFile);
      if Length(TEKFont) = 0 then
        SetIniString('Tera Term', 'TEKFont', 'Terminal,0,-8,255', iniFile);
      SetIniString('Tera Term', 'UnicodeAmbiguousWidth', '2', iniFile);
      SetIniString('Tera Term', 'UnicodeEmojiOverride', 'on', iniFile);
      SetIniString('Tera Term', 'UnicodeEmojiWidth', '2', iniFile);
    end;
  $11: // Japanese
    begin
      if Length(Language) = 0 then
        SetIniString('Tera Term', 'Language', 'Japanese', iniFile);
      if Length(VTFont) = 0 then
        SetIniString('Tera Term', 'VTFont', 'ＭＳ ゴシック,0,-16,128', iniFile);
      if Length(TEKFont) = 0 then
        SetIniString('Tera Term', 'TEKFont', 'Terminal,0,-8,128', iniFile);
      SetIniString('Tera Term', 'UnicodeAmbiguousWidth', '2', iniFile);
      SetIniString('Tera Term', 'UnicodeEmojiOverride', 'on', iniFile);
      SetIniString('Tera Term', 'UnicodeEmojiWidth', '2', iniFile);
    end;
  $12: // Korean
    begin
      if Length(Language) = 0 then
        SetIniString('Tera Term', 'Language', 'Korean', iniFile);
      if Length(VTFont) = 0 then
        SetIniString('Tera Term', 'VTFont', 'Terminal,0,-12,255', iniFile);
      if Length(TEKFont) = 0 then
        SetIniString('Tera Term', 'TEKFont', 'Terminal,0,-8,255', iniFile);
      SetIniString('Tera Term', 'UnicodeAmbiguousWidth', '2', iniFile);
      SetIniString('Tera Term', 'UnicodeEmojiOverride', 'on', iniFile);
      SetIniString('Tera Term', 'UnicodeEmojiWidth', '2', iniFile);
    end;
  $19: // Russian
    begin
      if Length(Language) = 0 then
        SetIniString('Tera Term', 'Language', 'Russian', iniFile);
      if Length(VTFont) = 0 then
        SetIniString('Tera Term', 'VTFont', 'Terminal,0,-12,255', iniFile);
      if Length(TEKFont) = 0 then
        SetIniString('Tera Term', 'TEKFont', 'Terminal,0,-8,255', iniFile);
      SetIniString('Tera Term', 'UnicodeAmbiguousWidth', '1', iniFile);
      SetIniString('Tera Term', 'UnicodeEmojiOverride', 'off', iniFile);
      SetIniString('Tera Term', 'UnicodeEmojiWidth', '1', iniFile);
    end;
  else // Other
    begin

      if GetUILanguage = $409 then begin // en-US

        if Length(Language) = 0 then
          SetIniString('Tera Term', 'Language', 'UTF-8', iniFile);

      end else begin // Other

        if Length(Language) = 0 then
          SetIniString('Tera Term', 'Language', 'English', iniFile);

      end;

      if Length(VTFont) = 0 then
        SetIniString('Tera Term', 'VTFont', 'Terminal,0,-12,255', iniFile);
      if Length(TEKFont) = 0 then
        SetIniString('Tera Term', 'TEKFont', 'Terminal,0,-8,255', iniFile);
      SetIniString('Tera Term', 'UnicodeAmbiguousWidth', '1', iniFile);
      SetIniString('Tera Term', 'UnicodeEmojiOverride', 'off', iniFile);
      SetIniString('Tera Term', 'UnicodeEmojiWidth', '1', iniFile);
    end;
  end;

  case UILangFilePage.SelectedValueIndex of
    1:
      SetIniString('Tera Term', 'UILanguageFile', 'lang_utf16le\ja_JP.lng', iniFile);
    2:
      SetIniString('Tera Term', 'UILanguageFile', 'lang_utf16le\de_DE.lng', iniFile);
    3:
      SetIniString('Tera Term', 'UILanguageFile', 'lang_utf16le\fr_FR.lng', iniFile);
    4:
      SetIniString('Tera Term', 'UILanguageFile', 'lang_utf16le\ru_RU.lng', iniFile);
    5:
      SetIniString('Tera Term', 'UILanguageFile', 'lang_utf16le\ko_KR.lng', iniFile);
    6:
      SetIniString('Tera Term', 'UILanguageFile', 'lang_utf16le\zh_CN.lng', iniFile);
    7:
      SetIniString('Tera Term', 'UILanguageFile', 'lang_utf16le\zh_TW.lng', iniFile);
    8:
      SetIniString('Tera Term', 'UILanguageFile', 'lang_utf16le\es_ES.lng', iniFile);
    9:
      SetIniString('Tera Term', 'UILanguageFile', 'lang_utf16le\ta_IN.lng', iniFile);
    10:
      SetIniString('Tera Term', 'UILanguageFile', 'lang_utf16le\pt_BR.lng', iniFile);
    11:
      SetIniString('Tera Term', 'UILanguageFile', 'lang_utf16le\it_IT.lng', iniFile);
    12:
      SetIniString('Tera Term', 'UILanguageFile', 'lang_utf16le\tr_TR.lng', iniFile);
    else
      SetIniString('Tera Term', 'UILanguageFile', 'lang_utf16le\Default.lng', iniFile);
  end;

  if TCPPort = 0 then begin
    if WizardIsComponentSelected('TTSSH') then
      SetIniInt('Tera Term', 'TCPPort', 22, iniFile)
    else
      SetIniInt('Tera Term', 'TCPPort', 23, iniFile);
  end;

  if ViewlogEditor = 'notepad.exe' then begin
    ViewlogEditor := ExpandConstant('{win}') + '\' + 'notepad.exe';
    SetIniString('Tera Term', 'ViewlogEditor', ViewlogEditor, iniFile);
  end;

  if WizardIsComponentSelected('TTSSH') then
    begin
      // これまでの TERATERM.INI のデフォルト値と同じ値なら、最新のデフォルト値で上書きする
      // 新しく追加した方式が disable line より後ろに行ってしまう現象への対処
      CipherOrder := GetIniString('TTSSH', 'CipherOrder', '', iniFile);
      if (CompareStr(CipherOrder, 'MLK>H:J=G9I<F8C7D;EB30A@?62') = 0) or
         (CompareStr(CipherOrder, 'K>H:J=G9I<F8C7D;EB30A@?62') = 0) or
         (CompareStr(CipherOrder, 'K>H:J=G9I<F8C7D;A@?EB3062') = 0) or
         (CompareStr(CipherOrder, '>:=9<8C7D;A@?EB3062') = 0) or
         (CompareStr(CipherOrder, '>:=9<87;A@?B3026') = 0) or
         (CompareStr(CipherOrder, '>:=9<87;A@?3026') = 0) or
         (CompareStr(CipherOrder, '>:=9<87;?3026') = 0) or
         (CompareStr(CipherOrder, '<8=9>:7;3026') = 0) or
         (CompareStr(CipherOrder, '87;9:<=>3026') = 0) or
         (CompareStr(CipherOrder, '87;9:3026') = 0) or
         (CompareStr(CipherOrder, '873026') = 0) then
        SetIniString('TTSSH', 'CipherOrder', 'MKN>H:J=G9LI<F8C7D;EB30A@?62', iniFile)
    end;

end;

procedure InitializeWizard;
var
  UserConfigDir             : String;
  UILangFilePageCaption     : String;
  UILangFilePageDescription : String;
  UILangFilePageSubCaption  : String;
  UILangFilePageSubCaption2 : String;
  UILangFilePageNone        : String;
  UILangFilePageJapanese    : String;
  UILangFilePageGerman      : String;
  UILangFilePageFrench      : String;
  UILangFilePageRussian     : String;
  UILangFilePageKorean      : String;
  UILangFilePageChinese     : String;
  UILangFilePageTChinese    : String;
  UILangFilePageSpanish     : String;
  UILangFilePageTamil       : String;
  UILangFilePagePortuguese  : String;
  UILangFilePageItalian     : String;
  UILangFilePageTurkish     : String;
begin
  UserConfigDir             := ExpandConstant('{userappdata}\teraterm5');
  UILangFilePageCaption     := CustomMessage('msg_language_caption');
  UILangFilePageDescription := CustomMessage('msg_language_description');
  UILangFilePageSubCaption  := CustomMessage('msg_language_subcaption');
  UILangFilePageSubCaption2 := CustomMessage('msg_language_subcaption2');
  UILangFilePageNone        := CustomMessage('msg_language_none');
  UILangFilePageJapanese    := CustomMessage('msg_language_japanese');
  UILangFilePageGerman      := CustomMessage('msg_language_german');
  UILangFilePageFrench      := CustomMessage('msg_language_french');
  UILangFilePageRussian     := CustomMessage('msg_language_russian');
  UILangFilePageKorean      := CustomMessage('msg_language_korean');
  UILangFilePageChinese     := CustomMessage('msg_language_chinese');
  UILangFilePageTChinese    := CustomMessage('msg_language_tchinese');
  UILangFilePageSpanish     := CustomMessage('msg_language_spanish');
  UILangFilePageTamil       := CustomMessage('msg_language_tamil');
  UILangFilePagePortuguese  := CustomMessage('msg_language_portuguese');
  UILangFilePageItalian     := CustomMessage('msg_language_italian');
  UILangFilePageTurkish     := CustomMessage('msg_language_turkish');

  if FileExists(UserConfigDir + '\TERATERM.INI') then
  begin
    UILangFilePageSubCaption := UILangFilePageSubCaption + #13#10 + ExpandConstant(UILangFilePageSubCaption2)
  end;
  UILangFilePage := CreateInputOptionPage(wpSelectComponents,
    UILangFilePageCaption, UILangFilePageDescription,
    UILangFilePageSubCaption, True, False);
  UILangFilePage.Add(UILangFilePageNone);
  UILangFilePage.Add(UILangFilePageJapanese);
  UILangFilePage.Add(UILangFilePageGerman);
  UILangFilePage.Add(UILangFilePageFrench);
  UILangFilePage.Add(UILangFilePageRussian);
  UILangFilePage.Add(UILangFilePageKorean);
  UILangFilePage.Add(UILangFilePageChinese);
  UILangFilePage.Add(UILangFilePageTChinese);
  UILangFilePage.Add(UILangFilePageSpanish);
  UILangFilePage.Add(UILangFilePageTamil);
  UILangFilePage.Add(UILangFilePagePortuguese);
  UILangFilePage.Add(UILangFilePageItalian);
  UILangFilePage.Add(UILangFilePageTurkish);

  case GetUILanguage and $3FF of
    $11: // Japanese
     UILangFilePage.SelectedValueIndex := 1;
    $07: // German
      UILangFilePage.SelectedValueIndex := 2;
    $0C: // French
      UILangFilePage.SelectedValueIndex := 3;
    $19: // Russian
      UILangFilePage.SelectedValueIndex := 4;
    $12: // Korean
      UILangFilePage.SelectedValueIndex := 5;
    $04: // Chinese
      begin
        case GetUILanguage of
          $0004, $7800, $0804, $1004: // zh-Hans, zh, zh-CN, zh-SG
            // Chinese (Simplified)
            UILangFilePage.SelectedValueIndex := 6;
        else
          // Chinese (Traditional)
          UILangFilePage.SelectedValueIndex := 7;
        end;
      end;
    $0A: // Spanish
      UILangFilePage.SelectedValueIndex := 8;
    $49: // Tamil
      UILangFilePage.SelectedValueIndex := 9;
    $16: // Portuguese
      begin
        case GetUILanguage of
          $0416: // pt-BR
            UILangFilePage.SelectedValueIndex := 10;
        else
          // pt-PT and other Portuguese file is not created. pt-BR is used instead.
          UILangFilePage.SelectedValueIndex := 10;
        end;
      end;
    $10: // Italian
      UILangFilePage.SelectedValueIndex := 11;
    $1F: // Turkish
      UILangFilePage.SelectedValueIndex := 12;
  else // Other
    UILangFilePage.SelectedValueIndex := 0;
  end;

end;

function NextButtonClick(CurPageID: Integer): Boolean;
var
  iniFile      : String;
  ErrMsg       : String;
begin
  Result := True;

  case CurPageID of

    wpSelectDir:
      begin

        ErrMsg := CheckAppsUsing();
        if Length(ErrMsg) > 0 then
          begin
            MsgBox(Format(CustomMessage('msg_AppRunningError'), [ErrMsg]), mbError, MB_OK);
            Result := False;
          end
        else
        // -1: goto next. Turn over to Inno Setup.
        //  0: goto next. No problem.
        // NOP
      end;

    wpSelectComponents:
      begin

        if FileExists(GetDefaultIniFileName()) then
        begin
          iniFile := Lowercase(GetIniString('Tera Term', 'UILanguageFile', '', GetDefaultIniFilename()));
          if iniFile = 'lang_utf16le\ja_JP.lng' then
            UILangFilePage.SelectedValueIndex := 1
          else if iniFile = 'lang_utf16le\de_DE.lng' then
            UILangFilePage.SelectedValueIndex := 2
          else if iniFile = 'lang_utf16le\fr_FR.lng' then
            UILangFilePage.SelectedValueIndex := 3
          else if iniFile = 'lang_utf16le\ru_RU.lng' then
            UILangFilePage.SelectedValueIndex := 4
          else if iniFile = 'lang_utf16le\ko_KR.lng' then
            UILangFilePage.SelectedValueIndex := 5
          else if iniFile = 'lang_utf16le\zh_CN.lng' then
            UILangFilePage.SelectedValueIndex := 6
          else if iniFile = 'lang_utf16le\zh_TW.lng' then
            UILangFilePage.SelectedValueIndex := 7
          else if iniFile = 'lang_utf16le\es_ES.lng' then
            UILangFilePage.SelectedValueIndex := 8
          else if iniFile = 'lang_utf16le\ta_IN.lng' then
            UILangFilePage.SelectedValueIndex := 9
          else if iniFile = 'lang_utf16le\pt_BR.lng' then
            UILangFilePage.SelectedValueIndex := 10
          else if iniFile = 'lang_utf16le\it_IT.lng' then
            UILangFilePage.SelectedValueIndex := 11
          else if iniFile = 'lang_utf16le\tr_TR.lng' then
            UILangFilePage.SelectedValueIndex := 12
          else
            UILangFilePage.SelectedValueIndex := 0;
        end;

      end;
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  iniFile : String;
  strValue : String;
begin
  case CurStep of
    ssPostInstall:
      begin
        iniFile := GetDefaultIniFilename();
        SetIniFile(iniFile);

        // HKEY_CURRENT_USER への設定は HKEY_LOCAL_MACHINE より優先されて邪魔になるので削除する
        //   専用拡張子なので丸ごと削除する
        RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, 'Software\Classes\.ttl');
        RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, 'Software\Classes\.tty');
        //   ProgId を削除する
        //     TeraTerm.MacroFile という名前は変わらないが、 HKLM で使う
        //     TTYRecordFile という名前は廃止した
        RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, 'Software\Classes\TeraTerm.MacroFile');
        RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, 'Software\Classes\TTYRecordFile');
        //  Tera Term のものが登録されていたらプロトコルから削除する
        strValue := '';
        if RegQueryStringValue(HKEY_CURRENT_USER, 'Software\Classes\telnet\shell', '', strValue) then
        begin
          if Pos('Open with Tera Term', strValue) > 0 then
          begin
            RegDeleteValue(HKEY_CURRENT_USER, 'Software\Classes\telnet\shell', '');
          end
        end;
        strValue := '';
        if RegQueryStringValue(HKEY_CURRENT_USER, 'Software\Classes\telnet\shell\Open with Tera Term\command', '', strValue) then
        begin
          if Pos('ttermpro.exe', strValue) > 0 then
          begin
            RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, 'Software\Classes\telnet\shell\Open with Tera Term');
            RegDeleteKeyIfEmpty(HKEY_CURRENT_USER, 'Software\Classes\telnet\shell');
            RegDeleteKeyIfEmpty(HKEY_CURRENT_USER, 'Software\Classes\telnet');
          end
        end;
        strValue := '';
        if RegQueryStringValue(HKEY_CURRENT_USER, 'Software\Classes\ssh\DefaultIcon', '', strValue) then
        begin
          if Pos('ttxssh.dll', strValue) > 0 then
          begin
            RegDeleteValue(HKEY_CURRENT_USER, 'Software\Classes\ssh\DefaultIcon', '');
            RegDeleteKeyIfEmpty(HKEY_CURRENT_USER, 'Software\Classes\ssh\DefaultIcon');
          end;
        end;
        strValue := '';
        if RegQueryStringValue(HKEY_CURRENT_USER, 'Software\Classes\ssh\shell\open\command', '', strValue) then
        begin
          if Pos('ttermpro.exe', strValue) > 0 then
          begin
            RegDeleteValue(HKEY_CURRENT_USER, 'Software\Classes\ssh\shell\open\command', '');
            RegDeleteValue(HKEY_CURRENT_USER, 'Software\Classes\ssh', 'EditFlags');
            RegDeleteValue(HKEY_CURRENT_USER, 'Software\Classes\ssh', 'URL Protocol');
            RegDeleteValue(HKEY_CURRENT_USER, 'Software\Classes\ssh', '');
            RegDeleteKeyIfEmpty(HKEY_CURRENT_USER, 'Software\Classes\ssh\shell\open\command');
            RegDeleteKeyIfEmpty(HKEY_CURRENT_USER, 'Software\Classes\ssh\shell\open');
            RegDeleteKeyIfEmpty(HKEY_CURRENT_USER, 'Software\Classes\ssh\shell');
            RegDeleteKeyIfEmpty(HKEY_CURRENT_USER, 'Software\Classes\ssh');
          end;
        end;
        strValue := '';
        if RegQueryStringValue(HKEY_CURRENT_USER, 'Software\Classes\slogin\DefaultIcon', '', strValue) then
        begin
          if Pos('ttxssh.dll', strValue) > 0 then
          begin
            RegDeleteValue(HKEY_CURRENT_USER, 'Software\Classes\slogin\DefaultIcon', '');
            RegDeleteKeyIfEmpty(HKEY_CURRENT_USER, 'Software\Classes\slogin\DefaultIcon');
          end;
        end;
        strValue := '';
        if RegQueryStringValue(HKEY_CURRENT_USER, 'Software\Classes\slogin\shell\open\command', '', strValue) then
        begin
          if Pos('ttermpro.exe', strValue) > 0 then
          begin
            RegDeleteValue(HKEY_CURRENT_USER, 'Software\Classes\slogin\shell\open\command', '');
            RegDeleteValue(HKEY_CURRENT_USER, 'Software\Classes\slogin', 'EditFlags');
            RegDeleteValue(HKEY_CURRENT_USER, 'Software\Classes\slogin', 'URL Protocol');
            RegDeleteValue(HKEY_CURRENT_USER, 'Software\Classes\slogin', '');
            RegDeleteKeyIfEmpty(HKEY_CURRENT_USER, 'Software\Classes\slogin\shell\open\command');
            RegDeleteKeyIfEmpty(HKEY_CURRENT_USER, 'Software\Classes\slogin\shell\open');
            RegDeleteKeyIfEmpty(HKEY_CURRENT_USER, 'Software\Classes\slogin\shell');
            RegDeleteKeyIfEmpty(HKEY_CURRENT_USER, 'Software\Classes\slogin');
          end;
        end;

        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0);

      end; // ssPostInstall
   end; // case CurStep of
end; // CurStepChanged

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  ini     : array[0..1] of String;
  buf     : String;
  conf    : String;
  confmsg : String;
  app     : String;
  i, res  : Integer;
  silent  : Boolean;
begin
  case CurUninstallStep of
    usPostUninstall:
      begin
        ini[0] := '\lang\Default.lng';
        ini[1] := '\lang_utf16le\Default.lng';

        conf := CustomMessage('msg_del_confirm');
        app  := ExpandConstant('{app}');

        silent := false;
        for i := 0 to ParamCount() do
        begin
          if (CompareText('/SUPPRESSMSGBOXES', ParamStr(i)) = 0) then
            silent := true;
        end;

        if not silent then begin

          // delete config files
          for i := 0 to 1 do
          begin
            buf := app + ini[i];
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

        end;

        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0);

        // directory is deleted only if empty
        RemoveDir(app + '\lang');
        RemoveDir(app + '\lang_utf16le');
        RemoveDir(app);
      end;
  end;
end;

[InstallDelete]
; インストーラに含めたことがあり、のちに含めなくなったファイルを指定する。
; 新しいインストーラで上書きインストールしたあとのアンインストーラでは削除されないため。
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
Name: {app}\cygterm+-x86_64\cyglaunch.exe; Type: files
Name: {app}\ttpdlg.dll; Type: files
Name: {app}\ttpset.dll; Type: files
Name: {app}\ttptek.dll; Type: files

[UninstallDelete]
; cygterm.exe は cygterm+-x86_64\cygterm.exe か cygterm+-i686\cygterm.exe を
; スクリプトでコピーしたもので、自動でアンインストールされないため。
Name: {app}\cygterm.exe; Type: files
